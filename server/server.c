/*********************************************************************
 *  server/server.c  –  Servidor con Registro y Playlists por usuario
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

/* ───────────── Auxiliares de playlist ───────────── */
static void playlist_path(char *buf,size_t sz,const char *user)
{
    snprintf(buf,sz,"%s/playlists/%s.txt",DATADIR,user);
}

/* Registrar nuevo usuario */
static int register_user_server(const char *u,const char *hash)
{
    char path[256]; snprintf(path,sizeof path,"%s/users.txt",DATADIR);
    FILE *f=fopen(path,"a+"); if(!f) return 0;
    rewind(f);
    char line[256];
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        if(!strncmp(line,u,strlen(u))&&line[strlen(u)]==':'){
            fclose(f); return 0;              /* ya existe */
        }
    }
    fprintf(f,"%s:%s\n",u,hash);
    fclose(f); return 1;
}

/* -------------- Manejo de playlists -------------- */
static void playlist_handle(SharedData *d,const char *user)
{
    char *cmd=strtok(d->message+6,"|");
    char *arg=strtok(NULL,"|");
    char pfile[256]; playlist_path(pfile,sizeof pfile,user);

    /* SHOW / PLAY */
    if(!strcmp(cmd,"SHOW") || !strcmp(cmd,"PLAY")){
        FILE *fp=fopen(pfile,"r");
        if(!fp){ snprintf(d->message,MAX_MSG,"EMPTY"); return; }
        char res[MAX_MSG]="PLIST|",line[128]; int first=1;
        while(fgets(line,sizeof line,fp)){
            line[strcspn(line,"\n")]='\0';
            if(!first) strncat(res,";",MAX_MSG-strlen(res)-1);
            strncat(res,line,MAX_MSG-strlen(res)-1); first=0;
        }
        fclose(fp); strncpy(d->message,res,MAX_MSG);
    }
    /* ADD */
    else if(!strcmp(cmd,"ADD") && arg){
        FILE *fp=fopen(pfile,"a+");
        if(!fp){ snprintf(d->message,MAX_MSG,"ERROR"); return; }
        int dup=0; rewind(fp); char l[128];
        while(fgets(l,sizeof l,fp)){
            l[strcspn(l,"\n")]='\0';
            if(!strcmp(l,arg)){ dup=1; break; }
        }
        if(!dup) fprintf(fp,"%s\n",arg);
        fclose(fp); snprintf(d->message,MAX_MSG,"OK");
    }
    /* DEL  (→ fix buffer) */
    else if(!strcmp(cmd,"DEL") && arg){
        FILE *fp=fopen(pfile,"r");
        if(!fp){ snprintf(d->message,MAX_MSG,"ERROR"); return; }

        /* 256 (size of pfile) + 4 ".tmp" + 1 '\0' */
        char tmp[sizeof(pfile) + 5];
        snprintf(tmp,sizeof tmp,"%s.tmp",pfile);

        FILE *out=fopen(tmp,"w");
        char l[128];
        while(fgets(l,sizeof l,fp)){
            l[strcspn(l,"\n")]='\0';
            if(strcmp(l,arg)) fprintf(out,"%s\n",l);
        }
        fclose(fp); fclose(out);
        rename(tmp,pfile);
        snprintf(d->message,MAX_MSG,"OK");
    }
    else snprintf(d->message,MAX_MSG,"ERROR");
}

/* ───────────── Handlers de canciones ───────────── */
static void handle_songs_request(SharedData *d){
    char path[256]; snprintf(path,sizeof path,"%s/songs.txt",DATADIR);
    FILE *f=fopen(path,"r");
    if(!f){ snprintf(d->message,MAX_MSG,"ERROR|Cannot open songs.txt"); return; }

    char res[MAX_MSG]="SONGS|",line[128]; int first=1;
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        char *title=strtok(line,":");
        if(!first) strncat(res,";",MAX_MSG-strlen(res)-1);
        strncat(res,title,MAX_MSG-strlen(res)-1); first=0;
    }
    fclose(f); strncpy(d->message,res,MAX_MSG);
}
static void handle_search_request(SharedData *d){
    char *term=d->message+7;
    char path[256]; snprintf(path,sizeof path,"%s/songs.txt",DATADIR);
    FILE *f=fopen(path,"r"); if(!f){
        snprintf(d->message,MAX_MSG,"ERROR|Cannot open songs.txt"); return; }
    char res[MAX_MSG]="FOUND|",line[128]; int found=0;
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        if(strstr(line,term)){
            if(found) strncat(res,";",MAX_MSG-strlen(res)-1);
            char *title=strtok(line,":");
            strncat(res,title,MAX_MSG-strlen(res)-1); found=1;
        }
    }
    fclose(f);
    if(found) strncpy(d->message,res,MAX_MSG);
    else snprintf(d->message,MAX_MSG,"NOT_FOUND|No matches found");
}
static void handle_get_request(SharedData *d,sem_t *csem,sem_t *ssem){
    char *req=d->message+4;
    char list[256]; snprintf(list,sizeof list,"%s/songs.txt",DATADIR);
    FILE *f=fopen(list,"r");
    if(!f){ snprintf(d->message,MAX_MSG,"ERROR|songs.txt not found"); sem_post(ssem); return; }
    char line[256],mp3[256]="";
    while(fgets(line,sizeof line,f)){
        line[strcspn(line,"\n")]='\0';
        char *title=strtok(line,":"); strtok(NULL,":"); char *path=strtok(NULL,":");
        if(title&&path&&!strcmp(title,req)){ strncpy(mp3,path,sizeof mp3); break; }
    }
    fclose(f);
    if(!mp3[0]){ snprintf(d->message,MAX_MSG,"ERROR|Song not found"); sem_post(ssem); return; }
    FILE *fp=fopen(mp3,"rb");
    if(!fp){ snprintf(d->message,MAX_MSG,"ERROR|Cannot open file"); sem_post(ssem); return; }
    while(1){
        int n=fread(d->audio_chunk,1,AUDIO_CHUNK_SIZE,fp);
        d->chunk_size=n; d->is_last_chunk=feof(fp);
        sem_post(ssem); sem_wait(csem);
        if(d->is_last_chunk) break;
    }
    fclose(fp);
}

/* ───────────── Unregister ───────────── */
static void unregister_client(pid_t pid){
    sem_t *sem=sem_open(REGISTRY_SEM_NAME,0);
    int fd=shm_open(REGISTRY_SHM_NAME,O_RDWR,0666);
    if(sem==SEM_FAILED||fd==-1) return;
    sem_wait(sem);
    ClientRegistry *reg=mmap(NULL,sizeof(ClientRegistry),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    for(int i=0;i<reg->count;i++){
        if(reg->pids[i]==pid){
            for(int j=i;j<reg->count-1;j++) reg->pids[j]=reg->pids[j+1];
            reg->count--; break;
        }
    }
    munmap(reg,sizeof(ClientRegistry)); close(fd); sem_post(sem); sem_close(sem);
}

/* ───────────── Thread cliente ───────────── */
static void* handle_client(void *arg){
    pid_t pid=*(pid_t*)arg; free(arg);
    char shm[64],sc[64],ss[64]; generate_names(shm,sc,ss,pid);
    int fd=-1; for(int i=0;i<20;i++){
        fd=shm_open(shm,O_RDWR,0666); if(fd!=-1) break; usleep(500000);}
    if(fd==-1){ fprintf(stderr,"Open SHM fail %d\n",pid); pthread_exit(NULL); }

    SharedData *d=mmap(NULL,sizeof(SharedData),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    sem_t *csem=sem_open(sc,0),*ssem=sem_open(ss,0);
    char current_user[LOGIN_INPUT_MAX]={0};
    printf("Client %d connected\n",pid);

    while(sem_wait(csem)!=-1){
        /* REGISTER */
        if(!strncmp(d->message,"REGISTER|",9)){
            char *u=strtok(d->message+9,"|"); char *h=strtok(NULL,"|");
            int ok=(u&&h)?register_user_server(u,h):0;
            snprintf(d->message,MAX_MSG,ok?"OK":"ERROR");
            sem_post(ssem); continue;
        }
        /* LOGIN */
        else if(!strncmp(d->message,"LOGIN|",6)){
            char *u=strtok(d->message+6,"|"); char *p=strtok(NULL,"|");
            if(u&&p&&authenticate(u,p)){
                strncpy(current_user,u,sizeof current_user);
                snprintf(d->message,MAX_MSG,"OK"); printf("Auth %s\n",u);
            } else snprintf(d->message,MAX_MSG,"ERROR");
        }
        /* PLAYLIST */
        else if(!strncmp(d->message,"PLIST|",6)){
            if(!current_user[0]) snprintf(d->message,MAX_MSG,"ERROR");
            else playlist_handle(d,current_user);
            sem_post(ssem); continue;
        }
        /* LOGOUT */
        else if(!strncmp(d->message,"LOGOUT",6)){
            printf("Logout %d\n",pid); unregister_client(pid); break;
        }
        /* Canciones */
        else if(!strncmp(d->message,"SONGS",5))   handle_songs_request(d);
        else if(!strncmp(d->message,"SEARCH|",7)) handle_search_request(d);
        else if(!strncmp(d->message,"GET|",4)){   handle_get_request(d,csem,ssem); continue; }

        sem_post(ssem);
    }
    munmap(d,sizeof(SharedData)); close(fd);
    sem_close(csem); sem_close(ssem);
    shm_unlink(shm); sem_unlink(sc); sem_unlink(ss);
    return NULL;
}

/* ───────────── Limpieza SIGINT ───────────── */
static void cleanup(int sig){
    puts("\nSIGINT -> cleaning…");
    sem_unlink(REGISTRY_SEM_NAME);
    shm_unlink(REGISTRY_SHM_NAME);
    exit(0);
}

/* ───────────── main ───────────── */
int main(void){
    signal(SIGINT,cleanup);
    sem_unlink(REGISTRY_SEM_NAME);

    int fd=shm_open(REGISTRY_SHM_NAME,O_CREAT|O_RDWR,0666);
    ftruncate(fd,sizeof(ClientRegistry));
    ClientRegistry *reg=mmap(NULL,sizeof(ClientRegistry),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    reg->count=0; puts("Server ready.");

    pid_t handled[MAX_CLIENTS]={0}; int hc=0;
    while(1){
        for(int i=0;i<reg->count;i++){
            pid_t pid=reg->pids[i],known=0;
            for(int j=0;j<hc;j++) if(handled[j]==pid){ known=1; break; }
            if(!known){
                pthread_t tid; pid_t *pp=malloc(sizeof(pid_t)); *pp=pid;
                if(!pthread_create(&tid,NULL,handle_client,pp)){
                    pthread_detach(tid); handled[hc++]=pid;
                } else free(pp);
            }
        }
        sleep(1);
    }
    return 0;
}
