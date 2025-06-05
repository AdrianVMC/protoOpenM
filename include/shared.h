#ifndef SHARED_H
#define SHARED_H

#include <unistd.h>

#define MAX_MSG 256
#define SHM_NAME_LEN 64
#define SEM_NAME_LEN 64

#define SHM_PREFIX "/shm_client_"
#define SEM_CLIENT_PREFIX "/sem_client_"
#define SEM_SERVER_PREFIX "/sem_server_"

typedef struct {
    char message[MAX_MSG];
} SharedData;

#endif