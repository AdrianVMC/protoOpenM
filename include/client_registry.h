#ifndef CLIENT_REGISTRY_H
#define CLIENT_REGISTRY_H

#include <sys/types.h>
#include <semaphore.h>

#define MAX_CLIENTS 10
#define REGISTRY_SHM_NAME "/client_registry"
#define REGISTRY_SEM_NAME "/registry_sem"

typedef struct {
    pid_t pids[MAX_CLIENTS];
    int count;
} ClientRegistry;

void register_client(pid_t pid);

#endif