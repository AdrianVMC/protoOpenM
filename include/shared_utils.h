#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <sys/types.h>

void generate_names(char *shm_name, char *sem_client, char *sem_server, pid_t pid);

#endif
