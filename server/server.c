/*********************************************************************
 *  server/server.c  –  Servidor multiproceso con registro de usuarios
 *********************************************************************/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#include "../include/shared.h"
#include "../include/shared_utils.h"
#include "../include/client_registry.h"
#include "../include/authenticate.h"

/* ──────────────────── Prototipos ──────────────────── */
static void handle_songs_request  (SharedData*);
static void handle_search_request (SharedData*);
static void handle_get_request    (SharedData*,sem_t*,sem_t*);
static void unregister_client     (pid_t);
static void* handle_client        (void*);
static void cleanup               (int);

/* ──────────────────── REGISTER (nuevo) ──────────────────── */
static int register_user_server(const char *user,const char *hash){
    char path[256]; snprintf(path,sizeof path,"%s/users.txt",DATADIR);
    FILE *f=fopen(path,"a+"); if(!f){ perror("users.txt"); return 0; }
    rewind(f);
    char line[256];
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        if(!strncmp(line,user,strlen(user)) && line[strlen(user)]==':'){
            fclose(f); return 0;               /* ya existe */
        }
    }
    fprintf(f,"%s:%s\n",user,hash);
    fclose(f); return 1;                       /* creado */
}

/* ──────────────────── SONGS ──────────────────── */
static void handle_songs_request(SharedData *d){
    char path[256]; snprintf(path,sizeof path,"%s/songs.txt",DATADIR);
    FILE *f=fopen(path,"r");
    if(!f){ snprintf(d->message,MAX_MSG,"ERROR|Cannot open songs.txt"); return; }

    char buffer[MAX_MSG]="SONGS|",line[128]; int first=1;
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        char *title=strtok(line,":");
        if(!first) strncat(buffer,";",MAX_MSG-strlen(buffer)-1);
        strncat(buffer,title,MAX_MSG-strlen(buffer)-1);
        first=0;
    }
    fclose(f); strncpy(d->message,buffer,MAX_MSG);
}

/* ──────────────────── SEARCH ──────────────────── */
static void handle_search_request(SharedData *d){
    char *term=d->message+7;
    char path[256]; snprintf(path,sizeof path,"%s/songs.txt",DATADIR);
    FILE *f=fopen(path,"r");
    if(!f){ snprintf(d->message,MAX_MSG,"ERROR|Cannot open songs.txt"); return; }

    char result[MAX_MSG]="FOUND|",line[128]; int found=0;
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        if(strstr(line,term)){
            if(found) strncat(result,";",MAX_MSG-strlen(result)-1);
            char *title=strtok(line,":");
            strncat(result,title,MAX_MSG-strlen(result)-1);
            found=1;
        }
    }
    fclose(f);
    if(found) strncpy(d->message,result,MAX_MSG);
    else snprintf(d->message,MAX_MSG,"NOT_FOUND|No matches found");
}

/* ──────────────────── GET (envío de audio) ──────────────────── */
static void handle_get_request(SharedData *d,sem_t *csem,sem_t *ssem){
    char *req=d->message+4;
    char pathlist[256]; snprintf(pathlist,sizeof pathlist,"%s/songs.txt",DATADIR);
    FILE *f=fopen(pathlist,"r");
    if(!f){ snprintf(d->message,MAX_MSG,"ERROR|songs.txt not found"); sem_post(ssem); return; }

    char line[256],filepath[256]="";
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        char *title=strtok(line,":"); strtok(NULL,":");
        char *path=strtok(NULL,":");
        if(title&&path&&strcmp(title,req)==0){ strncpy(filepath,path,sizeof filepath); break; }
    }
    fclose(f);
    if(!filepath[0]){ snprintf(d->message,MAX_MSG,"ERROR|Song not found"); sem_post(ssem); return; }

    FILE *fp=fopen(filepath,"rb");
    if(!fp){ snprintf(d->message,MAX_MSG,"ERROR|Cannot open file"); sem_post(ssem); return; }

    while(1){
        int n=fread(d->audio_chunk,1,AUDIO_CHUNK_SIZE,fp);
        d->chunk_size=n; d->is_last_chunk=feof(fp)?1:0;
        sem_post(ssem); sem_wait(csem);
        if(d->is_last_chunk) break;
    }
    fclose(fp);
}

/* ──────────────────── UNREGISTER ──────────────────── */
static void unregister_client(pid_t pid){
    sem_t *sem=sem_open(REGISTRY_SEM_NAME,0);
    int shm_fd=shm_open(REGISTRY_SHM_NAME,O_RDWR,0666);
    if(sem==SEM_FAILED||shm_fd==-1) return;

    sem_wait(sem);
    ClientRegistry *reg=mmap(NULL,sizeof(ClientRegistry),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    for(int i=0;i<reg->count;i++){
        if(reg->pids[i]==pid){
            for(int j=i;j<reg->count-1;j++) reg->pids[j]=reg->pids[j+1];
            reg->count--; break;
        }
    }
    munmap(reg,sizeof(ClientRegistry)); close(shm_fd); sem_post(sem); sem_close(sem);
}

/* ──────────────────── Hilo por cliente ──────────────────── */
static void* handle_client(void *arg){
    pid_t pid=*(pid_t*)arg; free(arg);
    char shm_name[64],sem_c[64],sem_s[64]; generate_names(shm_name,sem_c,sem_s,pid);

    int shm_fd=-1; for(int i=0;i<20;i++){
        shm_fd=shm_open(shm_name,O_RDWR,0666); if(shm_fd!=-1) break; usleep(500000);}
    if(shm_fd==-1){ fprintf(stderr,"Cannot open SHM for %d\n",pid); pthread_exit(NULL); }

    SharedData *d=mmap(NULL,sizeof(SharedData),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    sem_t *csem=sem_open(sem_c,0),*ssem=sem_open(sem_s,0);
    printf("Attending client PID %d\n",pid);

    while(sem_wait(csem)!=-1){
        /* REGISTER */
        if(!strncmp(d->message,"REGISTER|",9)){
            char *u=strtok(d->message+9,"|");
            char *h=strtok(NULL,"|");
            int ok=(u&&h)?register_user_server(u,h):0;
            snprintf(d->message,MAX_MSG,ok?"OK":"ERROR");
            sem_post(ssem); continue;
        }
        /* LOGIN */
        else if(!strncmp(d->message,"LOGIN|",6)){
            char *u=strtok(d->message+6,"|");
            char *p=strtok(NULL,"|");
            if(u&&p&&authenticate(u,p)){
                snprintf(d->message,MAX_MSG,"OK");
                printf("Verified client: %s\n",u);
            }else{
                snprintf(d->message,MAX_MSG,"ERROR");
                printf("Invalid login: %s\n",u?u:"NULL");
                unregister_client(pid);
            }
        }
        /* Otras peticiones */
        else if(!strncmp(d->message,"LOGOUT",6)){
            printf("Client %d logout\n",pid); unregister_client(pid); break;
        }
        else if(!strncmp(d->message,"SONGS",5))   handle_songs_request(d);
        else if(!strncmp(d->message,"SEARCH|",7)) handle_search_request(d);
        else if(!strncmp(d->message,"GET|",4)){   handle_get_request(d,csem,ssem); continue; }

        sem_post(ssem);
    }

    munmap(d,sizeof(SharedData)); close(shm_fd);
    sem_close(csem); sem_close(ssem);
    shm_unlink(shm_name); sem_unlink(sem_c); sem_unlink(sem_s);
    pthread_exit(NULL);
}

/* ──────────────────── Limpieza Ctrl-C ──────────────────── */
static void cleanup(int sig){
    puts("\nCleaning resources...");
    sem_unlink(REGISTRY_SEM_NAME);
    shm_unlink(REGISTRY_SHM_NAME);
    exit(0);
}

/* ──────────────────── main ──────────────────── */
int main(void){
    signal(SIGINT,cleanup);
    sem_unlink(REGISTRY_SEM_NAME);

    /* SHM para registro de clientes */
    int shm_fd=shm_open(REGISTRY_SHM_NAME,O_CREAT|O_RDWR,0666);
    ftruncate(shm_fd,sizeof(ClientRegistry));
    ClientRegistry *reg=mmap(NULL,sizeof(ClientRegistry),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    reg->count=0;
    printf("Server ready - waiting for clients…\n");

    pid_t handled[MAX_CLIENTS]={0}; int hcount=0;

    while(1){
        for(int i=0;i<reg->count;i++){
            pid_t pid=reg->pids[i]; int known=0;
            for(int j=0;j<hcount;j++) if(handled[j]==pid){ known=1; break; }
            if(!known){
                pthread_t tid; pid_t *pp=malloc(sizeof(pid_t)); *pp=pid;
                if(!pthread_create(&tid,NULL,handle_client,pp)){
                    pthread_detach(tid); handled[hcount++]=pid;
                } else free(pp);
            }
        }
        sleep(1);
    }
    munmap(reg,sizeof(ClientRegistry)); close(shm_fd); return 0;
}
