#include <ncurses.h>
#include "songs.h"

void ui_library(void){
    Song *arr; size_t n;
    if(songs_load(&arr,&n)<=0){
        mvprintw(2,4,"Biblioteca vacía. Pulse una tecla…"); getch(); return;
    }
    int sel=0,ch; keypad(stdscr,TRUE);
    while(1){
        clear(); mvprintw(1,3,"Biblioteca (%zu temas) – q para salir", n);
        for(size_t i=0;i<n;i++){
            if(i==sel) attron(A_REVERSE);
            mvprintw(3+i,5,"%s - %s", arr[i].title, arr[i].artist);
            if(i==sel) attroff(A_REVERSE);
        }
        ch=getch();
        if((ch=='q')||(ch=='\n')) break;
        else if(ch==KEY_UP   && sel)       --sel;
        else if(ch==KEY_DOWN && sel<n-1)   ++sel;
    }
    free(arr);
}
