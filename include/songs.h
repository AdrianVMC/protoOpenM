#ifndef SONGS_H
#define SONGS_H

#include <stddef.h>

#define TITLE_MAX  100
#define ARTIST_MAX 100

typedef struct {
    char title [TITLE_MAX];
    char artist[ARTIST_MAX];
    char path  [128];
} Song;


int  songs_init(void);
int  songs_add (const Song *s);
int  songs_load(Song **arr, size_t *count);

#endif
