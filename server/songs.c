#include "songs.h"
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

static const char *DB = "data/songs.dat";
static sem_t sem;

static void lock(void){   sem_wait (&sem); }
static void unlock(void){ sem_post (&sem); }

int songs_init(void){
    sem_init(&sem, 0, 1);
    FILE *f = fopen(DB, "ab");
    if(!f) return -1;
    fclose(f);
    return 0;
}

int songs_add(const Song *s){
    lock();
    FILE *f = fopen(DB,"ab");
    if(!f){ unlock(); return -1; }
    fwrite(s,sizeof(Song),1,f);
    fclose(f);
    unlock();
    return 0;
}

int songs_load(Song **arr, size_t *cnt){
    *arr = NULL; *cnt = 0;
    FILE *f = fopen(DB,"rb");
    if(!f) return -1;
    fseek(f,0,SEEK_END);
    long sz = ftell(f);
    rewind(f);

    size_t n = sz / sizeof(Song);
    if(n==0){ fclose(f); return 0; }

    *arr = malloc(sz);
    *cnt = n;
    fread(*arr, sizeof(Song), n, f);
    fclose(f);
    return (int)n;
}
