#include <stdio.h>
#include <unistd.h>
#include "../include/shared.h"

void generate_names(char *shm_name, char *sem_client, char *sem_server, pid_t pid) {
    snprintf(shm_name, SHM_NAME_LEN, SHM_PREFIX "%d", pid);
    snprintf(sem_client, SEM_NAME_LEN, SEM_CLIENT_PREFIX "%d", pid);
    snprintf(sem_server, SEM_NAME_LEN, SEM_SERVER_PREFIX "%d", pid);
}
