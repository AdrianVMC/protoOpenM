#ifndef SHARED_H
#define SHARED_H

#include "config.h"          /* ← ahora hereda todas las constantes */

/* ─────────── Estructura que comparten cliente y servidor ────────── */
typedef struct {
    char message[MAX_MSG];               /* comandos / respuestas texto */
    char audio_chunk[AUDIO_CHUNK_SIZE];  /* datos binarios de audio     */
    int  chunk_size;                     /* bytes válidos en audio_chunk*/
    int  is_last_chunk;                  /* 1 si es el último fragmento */
} SharedData;

#endif /* SHARED_H */
