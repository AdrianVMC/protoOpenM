#ifndef SHARED_H
#define SHARED_H

<<<<<<< Updated upstream
#define MAX_MSG 1024
#define AUDIO_CHUNK_SIZE 4096

#define SHM_NAME_LEN 64
#define SEM_NAME_LEN 64

#define SHM_PREFIX "/client_shm_"
#define SEM_CLIENT_PREFIX "/client_sem_"
#define SEM_SERVER_PREFIX "/server_sem_"

#define REGISTRY_SHM_NAME "/client_registry_shm"
#define REGISTRY_SEM_NAME "/client_registry_sem"
#define MAX_CLIENTS 10


typedef struct {
    char message[MAX_MSG];              // comandos y respuestas de texto
    char audio_chunk[AUDIO_CHUNK_SIZE]; // bloque de datos binarios
    int chunk_size;                     // cantidad real de bytes en audio_chunk
    int is_last_chunk;                 // 1 si es el último fragmento
=======
#include "config.h"

typedef struct {
    char message[MAX_MSG];
    char audio_chunk[AUDIO_CHUNK_SIZE];
    int  chunk_size;
    int  is_last_chunk;
>>>>>>> Stashed changes
} SharedData;

#endif
