/*********************************************************************
 *  client/cli.c  –  Cliente ncurses con login y registro integrados
 *  Compila con: gcc -Wall -pthread -Iinclude cli.c ... -lncursesw -lssl -lcrypto
 *********************************************************************/

#include "config.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#include "../include/shared.h"
#include "../include/shared_utils.h"
#include "../include/client_registry.h"
#include "../include/hash_utils.h"

/* ──────────────────── Constantes y menús ──────────────────── */
#define NUM_MAIN_OPTIONS 4
static const char *main_opts[NUM_MAIN_OPTIONS] = {
    "View songs", "Search song", "Reproduce song", "Logout"
};

/* ──────────────────── Utilidades varias ──────────────────── */
static inline void clear_term(void){ printf("\033[H\033[J"); }

/* Duración aproximada de mp3 con ffprobe */
static int mp3_duration(const char *file){
    char cmd[256]; snprintf(cmd,sizeof cmd,
        "ffprobe -v error -show_entries format=duration -of csv=p=0 \"%s\"",file);
    FILE *fp=popen(cmd,"r"); if(!fp) return 30;
    char buf[64]; if(!fgets(buf,sizeof buf,fp)){ pclose(fp); return 30; }
    pclose(fp); return (int)(atof(buf)+0.5);
}

/* Barra de progreso con controles P/R/Q */
static void show_progress_bar(int dur,pid_t pid_player){
    int paused=0,elapsed=0,total_pause=0;
    time_t start=time(NULL),pause_start=0;

    nodelay(stdscr,TRUE); curs_set(0); timeout(100);

    while(elapsed<dur){
        int ch=getch();
        if((ch=='p'||ch=='P')&&!paused){ kill(pid_player,SIGSTOP); paused=1; pause_start=time(NULL); }
        if((ch=='r'||ch=='R')&&paused){  kill(pid_player,SIGCONT); paused=0; total_pause+=(int)(time(NULL)-pause_start); }
        if(ch=='q'||ch=='Q'){ kill(pid_player,SIGTERM); waitpid(pid_player,NULL,0); break; }

        if(!paused) elapsed=(int)(time(NULL)-start-total_pause);

        clear();
        mvprintw(2,2,"Playing song... (%s)",paused?"Paused":"Playing");
        mvprintw(4,2,"[P]ause  [R]esume  [Q]uit");

        mvprintw(6,2,"[");
        int bar=50,pos=(elapsed*bar)/dur; if(pos>bar) pos=bar;
        for(int i=0;i<bar;i++) addch(i<pos? '=':' ');
        printw("] %d/%d sec",elapsed,dur);
        refresh();
    }
    nodelay(stdscr,FALSE); curs_set(1); waitpid(pid_player,NULL,0);
}

/* Descarga canción, reproduce, muestra barra */
static void download_and_play(SharedData *d,sem_t *csem,sem_t *ssem,const char *song){
    clear_term();
    snprintf(d->message,MAX_MSG,"GET|%s",song);
    sem_post(csem);

    char tmp[128]; snprintf(tmp,sizeof tmp,"/tmp/%s_tmp.mp3",song);
    FILE *fp=fopen(tmp,"wb");

    while(1){
        sem_wait(ssem);
        fwrite(d->audio_chunk,1,d->chunk_size,fp);
        sem_post(csem);
        if(d->is_last_chunk) break;
    }
    fclose(fp);

    initscr(); noecho(); cbreak();
    pid_t pid=fork();
    if(pid==0){ execlp("mpg123","mpg123","--quiet",tmp,NULL); perror("mpg123"); exit(1);}
    else{
        show_progress_bar(mp3_duration(tmp),pid);
    }
    endwin(); remove(tmp); clear_term();
}

/* ──────────────────── Vistas de menú ──────────────────── */
static void play_song_by_name(SharedData *d,sem_t *csem,sem_t *ssem){
    char name[64]; clear_term(); printf("Enter song name: ");
    fgets(name,sizeof name,stdin); name[strcspn(name,"\n")]='\0';
    if(name[0]=='\0'){ puts("No name entered. Enter to return."); getchar(); return; }
    download_and_play(d,csem,ssem,name);
    puts("\nPlayback finished. Enter to return..."); getchar();
}

static void view_songs(SharedData *d,sem_t *csem,sem_t *ssem){
    clear_term(); snprintf(d->message,MAX_MSG,"SONGS"); sem_post(csem); sem_wait(ssem);
    if(!strncmp(d->message,"SONGS|",6)){
        char list[MAX_MSG]; strncpy(list,d->message+6,sizeof list);
        char *song=strtok(list,";"); int idx=1; puts("\nAvailable songs:");
        while(song){ printf("  %2d. %s\n",idx++,song); song=strtok(NULL,";"); }
    } else puts(d->message);
    puts("\nEnter to return."); getchar();
}

static void search_song(SharedData *d,sem_t *csem,sem_t *ssem){
    char term[64]; clear_term(); printf("Enter name fragment: ");
    fgets(term,sizeof term,stdin); term[strcspn(term,"\n")]='\0';
    snprintf(d->message,MAX_MSG,"SEARCH|%s",term); sem_post(csem); sem_wait(ssem);
    if(!strncmp(d->message,"FOUND|",6)){
        char list[MAX_MSG]; strncpy(list,d->message+6,sizeof list);
        char *s=strtok(list,";"); int idx=1; puts("\nResults:");
        while(s){ printf("  %2d. %s\n",idx++,s); s=strtok(NULL,";"); }
    } else puts(d->message);
    puts("\nEnter to return."); getchar();
}

/* Menú principal ncurses */
static int show_main_menu_ncurses(void){
    initscr(); noecho(); cbreak(); curs_set(0); keypad(stdscr,TRUE);
    int hl=0,ch;
    while(1){
        clear(); mvprintw(1,2,"Main Menu");
        for(int i=0;i<NUM_MAIN_OPTIONS;i++){
            if(i==hl) attron(A_REVERSE);
            mvprintw(3+i,4,"%s",main_opts[i]);
            if(i==hl) attroff(A_REVERSE);
        }
        refresh();
        ch=getch();
        if(ch==KEY_UP)   hl=(hl==0)?NUM_MAIN_OPTIONS-1:hl-1;
        if(ch==KEY_DOWN) hl=(hl+1)%NUM_MAIN_OPTIONS;
        if(ch=='\n'){ endwin(); return hl; }
        if(ch==27){ endwin(); return -1; }
    }
}

/* ──────────────────── Registro remoto ──────────────────── */
static void register_user_remote(const char *u,const char *p,
                                 SharedData *d,sem_t *csem,sem_t *ssem){
    char hash[HASH_STRING_LENGTH]; hash_sha256(p,hash);
    snprintf(d->message,MAX_MSG,"REGISTER|%s|%s",u,hash);
    sem_post(csem); sem_wait(ssem);
    puts(strcmp(d->message,"OK")==0?
         "\nUser created!  Enter to continue..." :
         "\nRegister failed (user exists?)  Enter to continue...");
    getchar();
}

/* Interfaz registro ncurses */
static void register_user_interface(char *u,char *p,
                                    SharedData *d,sem_t *csem,sem_t *ssem){
    initscr(); cbreak(); noecho(); curs_set(1); keypad(stdscr,TRUE);
    int y=5,x=10; clear();
    mvprintw(y  ,x,"Register New User");
    mvprintw(y+2,x,"Username: "); echo(); move(y+2,x+10);
    getnstr(u,LOGIN_INPUT_MAX-1); noecho();
    mvprintw(y+4,x,"Password: "); move(y+4,x+10);
    int i=0,ch; while((ch=getch())!='\n'&&i<LOGIN_INPUT_MAX-1){
        if(ch>=32&&ch<=126){ p[i++]=ch; addch('*'); }
    } p[i]='\0'; endwin();
    if(u[0]&&p[0]) register_user_remote(u,p,d,csem,ssem);
}

/* ──────────────────── Login ──────────────────── */
static void login_interface(char *u,char *p,
                            SharedData *d,sem_t *csem,sem_t *ssem){
    clear_term(); initscr(); cbreak(); noecho(); curs_set(1); keypad(stdscr,TRUE);
    int y=5,x=10; clear();
    mvprintw(y,x,"Login");
    mvprintw(y+2,x,"Username: "); echo(); move(y+2,x+10);
    getnstr(u,LOGIN_INPUT_MAX-1); noecho();
    mvprintw(y+4,x,"Password: "); move(y+4,x+10);
    int i=0,ch; while((ch=getch())!='\n'&&i<LOGIN_INPUT_MAX-1){
        if(ch>=32&&ch<=126){ p[i++]=ch; addch('*'); }
    } p[i]='\0'; endwin();
    if(u[0]&&p[0]){
        snprintf(d->message,MAX_MSG,"LOGIN|%s|%s",u,p);
        sem_post(csem); sem_wait(ssem);
    } else { u[0]=p[0]='\0'; }
}

/* ──────────────────── Menú inicial ──────────────────── */
static int start_menu(void){
    const char *opt[]={"Login","Register","Exit"}; int hl=0,ch;
    initscr(); noecho(); cbreak(); curs_set(0); keypad(stdscr,TRUE);
    while(1){
        clear(); mvprintw(1,4,"Welcome to protoOpenM");
        for(int i=0;i<3;i++){ if(i==hl) attron(A_REVERSE);
            mvprintw(3+i,6,"%s",opt[i]); if(i==hl) attroff(A_REVERSE);}
        refresh(); ch=getch();
        if(ch==KEY_UP)   hl=(hl==0)?2:hl-1;
        if(ch==KEY_DOWN) hl=(hl+1)%3;
        if(ch=='\n'){ endwin(); return hl; }
    }
}

/* ──────────────────── main ──────────────────── */
int main(void){
    char user[LOGIN_INPUT_MAX]={0},pass[LOGIN_INPUT_MAX]={0};
    pid_t pid=getpid();
    char shm_name[64],sem_c[64],sem_s[64]; generate_names(shm_name,sem_c,sem_s,pid);

    int shm_fd=shm_open(shm_name,O_CREAT|O_RDWR,0666);
    ftruncate(shm_fd,sizeof(SharedData));
    SharedData *d=mmap(NULL,sizeof(SharedData),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    sem_t *csem=sem_open(sem_c,O_CREAT,0666,0),*ssem=sem_open(sem_s,O_CREAT,0666,0);
    register_client(pid);

    /* ----- menú inicial ----- */
    while(1){
        int ch=start_menu();
        if(ch==2) goto cleanup;
        if(ch==1){ register_user_interface(user,pass,d,csem,ssem); continue; }
        if(ch==0) break;
    }

    login_interface(user,pass,d,csem,ssem);
    if(user[0]=='\0'||pass[0]=='\0'||strcmp(d->message,"OK")!=0){
        puts("Authentication failed or cancelled."); goto cleanup;
    }
    printf("\nWelcome, %s\n",user);

    /* ----- menú principal ----- */
    int quit=0; while(!quit){
        int opt=show_main_menu_ncurses();
        switch(opt){
            case 0: view_songs(d,csem,ssem); break;
            case 1: search_song(d,csem,ssem); break;
            case 2: play_song_by_name(d,csem,ssem); break;
            case 3: snprintf(d->message,MAX_MSG,"LOGOUT"); sem_post(csem); quit=1; break;
            default: quit=1; break;
        }
    }

cleanup:
    munmap(d,sizeof(SharedData)); close(shm_fd);
    sem_close(csem); sem_close(ssem);
    shm_unlink(shm_name); sem_unlink(sem_c); sem_unlink(sem_s);
    return 0;
}
