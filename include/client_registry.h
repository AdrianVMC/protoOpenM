#ifndef CLIENT_REGISTRY_H
#define CLIENT_REGISTRY_H

#include <sys/types.h>
#include <semaphore.h>
#include <shared.h>

#define MAX_CLIENTS 10


typedef struct {
    pid_t pids[MAX_CLIENTS];
    int count;
} ClientRegistry;

void register_client(pid_t pid);

#endif