/*********************************************************************
 *  client/cli.c  –  Cliente con Login/Registro y Playlists
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

/* ───── Lectura mascara ───── */
static void read_masked(char *dst,int max,int y,int x){
    int i=0,ch; move(y,x);
    while((ch=getch())!='\n'){
        if((ch==KEY_BACKSPACE||ch==127)&&i){ --i; mvaddch(y,x+i,' '); move(y,x+i);}
        else if(ch>=32&&ch<=126&&i<max-1){ dst[i++]=ch; mvaddch(y,x+i-1,'*'); }
        refresh();
    } dst[i]='\0';
}

/* ───── Menú principal ───── */
#define NUM_MAIN 5
static const char *main_opt[NUM_MAIN]={
    "View songs","Search song","Reproduce song","My Playlist","Logout"
};
static inline void cls(void){ printf("\033[H\033[J"); }

/* ffprobe duración */
static int mp3_len(const char *f){
    char c[256]; snprintf(c,sizeof c,
        "ffprobe -v error -show_entries format=duration -of csv=p=0 \"%s\"",f);
    FILE *fp=popen(c,"r"); if(!fp) return 30;
    char b[64]; if(!fgets(b,sizeof b,fp)){ pclose(fp); return 30;} pclose(fp);
    return (int)(atof(b)+0.5);
}

/* Barra de progreso */
static void bar(int len,pid_t pid){
    int pause=0,elap=0,tot=0; time_t st=time(NULL),ps=0;
    nodelay(stdscr,TRUE); curs_set(0); timeout(100);
    while(elap<len){
        int ch=getch();
        if((ch=='p'||ch=='P')&&!pause){ kill(pid,SIGSTOP); pause=1; ps=time(NULL);}
        else if((ch=='r'||ch=='R')&&pause){ kill(pid,SIGCONT); pause=0; tot+=(int)(time(NULL)-ps);}
        else if(ch=='q'||ch=='Q'){ kill(pid,SIGTERM); waitpid(pid,NULL,0); break; }
        if(!pause) elap=(int)(time(NULL)-st-tot);
        clear(); mvprintw(2,2,"Playing (%s)",pause?"Paused":"Playing");
        mvprintw(4,2,"[P]ause [R]esume [Q]uit");
        mvprintw(6,2,"[");
        int bw=50,pos=(elap*bw)/len;
        for(int i=0;i<bw;i++) addch(i<pos?'=':' ');
        printw("] %d/%d sec",elap,len); refresh();
    }
    nodelay(stdscr,FALSE); curs_set(1); waitpid(pid,NULL,0);
}

/* Descarga y reproduce */
static void play_download(SharedData *d,sem_t *c,sem_t *s,const char *song){
    cls(); snprintf(d->message,MAX_MSG,"GET|%s",song); sem_post(c);
    char tmp[128]; snprintf(tmp,sizeof tmp,"/tmp/%s_tmp.mp3",song);
    FILE *fp=fopen(tmp,"wb");
    while(1){ sem_wait(s); fwrite(d->audio_chunk,1,d->chunk_size,fp); sem_post(c);
              if(d->is_last_chunk) break;}
    fclose(fp); initscr(); noecho(); cbreak();
    pid_t pid=fork();
    if(pid==0){ execlp("mpg123","mpg123","--quiet",tmp,NULL); perror("mpg123"); exit(1);}
    else bar(mp3_len(tmp),pid);
    endwin(); remove(tmp); cls();
}

/* View */
static void view(SharedData *d,sem_t *c,sem_t *s){
    cls(); snprintf(d->message,MAX_MSG,"SONGS"); sem_post(c); sem_wait(s);
    if(!strncmp(d->message,"SONGS|",6)){
        char list[MAX_MSG]; strncpy(list,d->message+6,sizeof list);
        char *t=strtok(list,";"); int n=1; puts("\nAvailable:");
        while(t){ printf("  %2d. %s\n",n++,t); t=strtok(NULL,";"); }
    } else puts(d->message);
    puts("\nEnter to return."); getchar();
}
/* Search */
static void search(SharedData *d,sem_t *c,sem_t *s){
    char term[64]; cls(); printf("Search term: ");
    fgets(term,sizeof term,stdin); term[strcspn(term,"\n")]='\0';
    snprintf(d->message,MAX_MSG,"SEARCH|%s",term); sem_post(c); sem_wait(s);
    if(!strncmp(d->message,"FOUND|",6)){
        char list[MAX_MSG]; strncpy(list,d->message+6,sizeof list);
        char *t=strtok(list,";"); int n=1; puts("\nResults:");
        while(t){ printf("  %2d. %s\n",n++,t); t=strtok(NULL,";"); }
    } else puts(d->message);
    puts("\nEnter."); getchar();
}
/* Play single */
static void play_single(SharedData *d,sem_t *c,sem_t *s){
    char n[64]; cls(); printf("Song title: "); fgets(n,sizeof n,stdin);
    n[strcspn(n,"\n")]='\0'; if(!n[0]) return;
    play_download(d,c,s,n); puts("\nDone. Enter."); getchar();
}
/* ───── Reproduce toda la playlist ───── */
static void play_playlist(SharedData *d, sem_t *csem, sem_t *ssem)
{
    /* 1. Pedir la lista */
    snprintf(d->message, MAX_MSG, "PLIST|PLAY");
    sem_post(csem); sem_wait(ssem);

    if (strcmp(d->message, "EMPTY") == 0) {
        puts("\n[Playlist vacía]  Enter para volver…"); getchar();
        return;
    }
    if (strncmp(d->message, "PLIST|", 6) != 0) {
        puts("\n[Error al obtener la playlist]  Enter…"); getchar();
        return;
    }

    /* 2. Parsear títulos */
    char list[MAX_MSG];
    strncpy(list, d->message + 6, sizeof list);
    char *song = strtok(list, ";");
    int track = 1, total = 0;

    /* contar total para mensaje bonito */
    for (char *tmp = song; tmp; tmp = strtok(NULL, ";")) ++total;
    strtok(list, ";");                     /* reset strtok: 1.ª canción */

    while (song) {
        printf("\n-- Reproduciendo %d/%d: %s --\n", track, total, song);
        play_download(d, csem, ssem, song);
        ++track;
        song = strtok(NULL, ";");
    }
    puts("\n[Playlist terminada]  Enter para volver…"); getchar();
}

/* Playlist UI */
static void pl_menu(SharedData *d,sem_t *c,sem_t *s){
    while(1){
        cls(); printf("Playlist\n1) Show\n2) Add\n3) Del\n4) Play\n0) Back\nOption: ");
        int op; scanf("%d%*c",&op); if(op==0) break;
        char t[128];
        switch(op){
            case 1:
                snprintf(d->message,MAX_MSG,"PLIST|SHOW"); sem_post(c); sem_wait(s);
                if(!strncmp(d->message,"PLIST|",6)){
                    char list[MAX_MSG]; strncpy(list,d->message+6,sizeof list);
                    char *p=strtok(list,";"); int n=1; puts("\nPlaylist:");
                    while(p){ printf("  %2d. %s\n",n++,p); p=strtok(NULL,";"); }
                } else puts("(empty)");
                break;
            case 2:
                printf("Song title: "); fgets(t,sizeof t,stdin);
                t[strcspn(t,"\n")]='\0';
                snprintf(d->message,MAX_MSG,"PLIST|ADD|%s",t); sem_post(c); sem_wait(s);
                puts("Added (if exists).");
                break;
            case 3:
                printf("Song title: "); fgets(t,sizeof t,stdin);
                t[strcspn(t,"\n")]='\0';
                snprintf(d->message,MAX_MSG,"PLIST|DEL|%s",t); sem_post(c); sem_wait(s);
                puts("Deleted (if existed).");
                break;
   			case 4:
    	 		play_playlist(d, c, s);
	     		break;
       		 }
        puts("\nEnter..."); getchar();
    }
}

/* Menú principal ncurses */
static int menu_ncurses(void){
    initscr(); noecho(); cbreak(); curs_set(0); keypad(stdscr,TRUE);
    int hl=0,ch;
    while(1){
        clear(); mvprintw(1,2,"Main Menu");
        for(int i=0;i<NUM_MAIN;i++){
            if(i==hl) attron(A_REVERSE);
            mvprintw(3+i,4,"%s",main_opt[i]);
            if(i==hl) attroff(A_REVERSE);
        }
        refresh(); ch=getch();
        if(ch==KEY_UP)   hl=(hl==0)?NUM_MAIN-1:hl-1;
        if(ch==KEY_DOWN) hl=(hl+1)%NUM_MAIN;
        if(ch=='\n'){ endwin(); return hl; }
        if(ch==27){ endwin(); return -1; }
    }
}

/* Registro remoto */
static void reg_remote(const char *u,const char *p,
                       SharedData *d,sem_t *c,sem_t *s){
    char hash[HASH_STRING_LENGTH]; hash_sha256(p,hash);
    snprintf(d->message,MAX_MSG,"REGISTER|%s|%s",u,hash);
    sem_post(c); sem_wait(s);
    puts(strcmp(d->message,"OK")==0? "Created!" : "Failed."); getchar();
}
/* Registro UI */
static void reg_ui(char *u,char *p,SharedData *d,sem_t *c,sem_t *s){
    initscr(); cbreak(); noecho(); curs_set(1); keypad(stdscr,TRUE);
    int y=5,x=10; clear();
    mvprintw(y,x,"Register");
    mvprintw(y+2,x,"Username: "); echo(); move(y+2,x+10); getnstr(u,LOGIN_INPUT_MAX-1); noecho();
    mvprintw(y+4,x,"Password: "); read_masked(p,LOGIN_INPUT_MAX,y+4,x+10); endwin();
    if(u[0]&&p[0]) reg_remote(u,p,d,c,s);
}

/* Login UI */
static void login_ui(char *u,char *p,SharedData *d,sem_t *c,sem_t *s){
    cls(); initscr(); cbreak(); noecho(); curs_set(1); keypad(stdscr,TRUE);
    int y=5,x=10; clear();
    mvprintw(y,x,"Login");
    mvprintw(y+2,x,"Username: "); echo(); move(y+2,x+10); getnstr(u,LOGIN_INPUT_MAX-1); noecho();
    mvprintw(y+4,x,"Password: "); read_masked(p,LOGIN_INPUT_MAX,y+4,x+10); endwin();
    if(u[0]&&p[0]){ snprintf(d->message,MAX_MSG,"LOGIN|%s|%s",u,p); sem_post(c); sem_wait(s); }
    else{ u[0]=p[0]='\0'; }
}

/* Menú inicial */
static int start_menu(void){
    const char *o[]={"Login","Register","Exit"}; int hl=0,ch;
    initscr(); noecho(); cbreak(); curs_set(0); keypad(stdscr,TRUE);
    while(1){
        clear(); mvprintw(1,4,"protoOpenM");
        for(int i=0;i<3;i++){ if(i==hl) attron(A_REVERSE);
            mvprintw(3+i,6,"%s",o[i]); if(i==hl) attroff(A_REVERSE);}
        refresh(); ch=getch();
        if(ch==KEY_UP)   hl=(hl==0)?2:hl-1;
        if(ch==KEY_DOWN) hl=(hl+1)%3;
        if(ch=='\n'){ endwin(); return hl; }
    }
}

/* ─── main ─── */
int main(void){
    char user[LOGIN_INPUT_MAX]={0},pass[LOGIN_INPUT_MAX]={0};
    pid_t pid=getpid();
    char shm[64],sc[64],ss[64]; generate_names(shm,sc,ss,pid);

    int fd=shm_open(shm,O_CREAT|O_RDWR,0666);
    ftruncate(fd,sizeof(SharedData));
    SharedData *d=mmap(NULL,sizeof(SharedData),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    sem_t *csem=sem_open(sc,O_CREAT,0666,0),*ssem=sem_open(ss,O_CREAT,0666,0);
    register_client(pid);

    while(1){
        int ch=start_menu();
        if(ch==2) goto end;
        if(ch==1){ reg_ui(user,pass,d,csem,ssem); continue; }
        if(ch==0) break;
    }
    login_ui(user,pass,d,csem,ssem);
    if(strcmp(d->message,"OK")!=0){ puts("Auth failed."); goto end; }

    int quit=0;
    while(!quit){
        int op=menu_ncurses();
        switch(op){
            case 0: view(d,csem,ssem); break;
            case 1: search(d,csem,ssem); break;
            case 2: play_single(d,csem,ssem); break;
            case 3: pl_menu(d,csem,ssem); break;
            case 4: snprintf(d->message,MAX_MSG,"LOGOUT"); sem_post(csem); quit=1; break;
            default: quit=1; break;
        }
    }

end:
    munmap(d,sizeof(SharedData)); close(fd);
    sem_close(csem); sem_close(ssem);
    shm_unlink(shm); sem_unlink(sc); sem_unlink(ss);
    return 0;
}
