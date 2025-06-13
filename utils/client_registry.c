#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include "../include/client_registry.h"

void register_client(pid_t pid) {
    sem_t *reg_sem = sem_open(REGISTRY_SEM_NAME, O_CREAT, 0666, 1);
    if (reg_sem == SEM_FAILED) {
        perror("sem_open (registry)");
        return;
    }

    sem_wait(reg_sem);

    int shm_fd = shm_open(REGISTRY_SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(ClientRegistry));
    ClientRegistry *reg = mmap(NULL, sizeof(ClientRegistry), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (reg == MAP_FAILED) {
        perror("mmap");
        sem_post(reg_sem);
        sem_close(reg_sem);
        return;
    }

    int already_registered = 0;
    for (int i = 0; i < reg->count; i++) {
        if (reg->pids[i] == pid) {
            already_registered = 1;
            break;
        }
    }

    if (!already_registered && reg->count < MAX_CLIENTS) {
        reg->pids[reg->count++] = pid;
        printf("Reg Client", pid);
    } else if (already_registered) {
        printf("Post Client", pid);
    } else {
        fprintf(stderr, "Full access.\n");
    }

    munmap(reg, sizeof(ClientRegistry));
    close(shm_fd);
    sem_post(reg_sem);
    sem_close(reg_sem);
}
