#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#include "../include/shared.h"
#include "../include/shared_utils.h"
#include "../include/client_registry.h"

void *handle_client(void *arg);
void cleanup(int sig);
void unregister_client(pid_t pid);

int authenticate(const char *username, const char *password) {
    FILE *file = fopen("data/users.txt", "r");
    if (!file) return 0;

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        char *file_user = strtok(line, ":");
        char *file_pass = strtok(NULL, ":");

        if (file_user && file_pass &&
            strcmp(file_user, username) == 0 &&
            strcmp(file_pass, password) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

void *handle_client(void *arg) {
    pid_t pid = *(pid_t *)arg;
    free(arg);

    char shm_name[64], sem_client[64], sem_server[64];
    generate_names(shm_name, sem_client, sem_server, pid);

    int shm_fd = -1;
    int intentos = 20;

    for (int i = 0; i < intentos; i++) {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (shm_fd != -1) break;
        usleep(500000); // espera 500ms
    }



    if (shm_fd == -1) {
        fprintf(stderr, "❌ No se pudo abrir la memoria compartida para PID %d\n", pid);
        pthread_exit(NULL);
    }


    SharedData *data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        pthread_exit(NULL);
    }

    sem_t *client_sem = sem_open(sem_client, 0);
    sem_t *server_sem = sem_open(sem_server, 0);
    if (client_sem == SEM_FAILED || server_sem == SEM_FAILED) {
        perror("sem_open");
        munmap(data, sizeof(SharedData));
        close(shm_fd);
        pthread_exit(NULL);
    }

    printf("🧵 Atendiendo cliente PID: %d\n", pid);

    while (1) {
        if (sem_wait(client_sem) == -1) {
            perror("sem_wait");
            break;
        }

        if (strncmp(data->message, "LOGIN|", 6) == 0) {
            char *username = strtok(data->message + 6, "|");
            char *password = strtok(NULL, "|");

            if (username && password && authenticate(username, password)) {
                snprintf(data->message, MAX_MSG, "OK");
                printf("🔑 Usuario autenticado: %s\n", username);
            } else {
                snprintf(data->message, MAX_MSG, "ERROR");
                printf("❌ Credenciales inválidas: %s\n", username ? username : "NULL");
            }
            sem_post(server_sem);
        }
        else if (strncmp(data->message, "LOGOUT", 6) == 0) {
            printf("👋 Cliente PID %d cerró sesión\n", pid);
            unregister_client(pid);
            break;
        }
        else if (strncmp(data->message, "SONGS", 5) == 0) {
            FILE *file = fopen("data/songs.txt", "r");
            if (!file) {
                snprintf(data->message, MAX_MSG, "ERROR|No se pudo abrir songs.txt");
            } else {
                char buffer[MAX_MSG] = "SONGS|";
                char line[128];
                int first = 1;

                while (fgets(line, sizeof(line), file)) {
                    line[strcspn(line, "\n")] = '\0';
                    if (!first) strncat(buffer, ";", MAX_MSG - strlen(buffer) - 1);
                    strncat(buffer, line, MAX_MSG - strlen(buffer) - 1);
                    first = 0;
                }
                fclose(file);
                strncpy(data->message, buffer, MAX_MSG);
            }
            sem_post(server_sem);
        }

    }


    munmap(data, sizeof(SharedData));
    close(shm_fd);
    sem_close(client_sem);
    sem_close(server_sem);
    pthread_exit(NULL);
}

void unregister_client(pid_t pid) {
    sem_t *reg_sem = sem_open(REGISTRY_SEM_NAME, 0);
    if (reg_sem == SEM_FAILED) {
        perror("sem_open (unregister)");
        return;
    }

    sem_wait(reg_sem);

    int shm_fd = shm_open(REGISTRY_SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open (unregister)");
        sem_post(reg_sem);
        sem_close(reg_sem);
        return;
    }

    ClientRegistry *reg = mmap(NULL, sizeof(ClientRegistry), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (reg == MAP_FAILED) {
        perror("mmap (unregister)");
        close(shm_fd);
        sem_post(reg_sem);
        sem_close(reg_sem);
        return;
    }

    for (int i = 0; i < reg->count; i++) {
        if (reg->pids[i] == pid) {
            for (int j = i; j < reg->count - 1; j++) {
                reg->pids[j] = reg->pids[j + 1];
            }
            reg->count--;
            printf("🗑️ Cliente eliminado del registro (PID: %d)\n", pid);
            break;
        }
    }

    munmap(reg, sizeof(ClientRegistry));
    close(shm_fd);
    sem_post(reg_sem);
    sem_close(reg_sem);
}



void cleanup(int sig) {
    printf("\n🧹 Limpiando recursos...\n");
    sem_unlink(REGISTRY_SEM_NAME);
    shm_unlink(REGISTRY_SHM_NAME);
    exit(0);
}

int main() {
    signal(SIGINT, cleanup);
    sem_unlink(REGISTRY_SEM_NAME);

    int shm_fd = shm_open(REGISTRY_SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(ClientRegistry));
    ClientRegistry *reg = mmap(NULL, sizeof(ClientRegistry), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (reg == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    reg->count = 0;

    printf("🟢 Servidor iniciado. Esperando clientes...\n");

    pid_t procesados[MAX_CLIENTS] = {0};
    int procesados_count = 0;

    while (1) {
        for (int i = 0; i < reg->count; i++) {
            pid_t pid = reg->pids[i];
            int ya_procesado = 0;

            for (int j = 0; j < procesados_count; j++) {
                if (procesados[j] == pid) {
                    ya_procesado = 1;
                    break;
                }
            }

            if (!ya_procesado) {
                pid_t *pid_ptr = malloc(sizeof(pid_t));
                *pid_ptr = pid;
                pthread_t tid;
                if (pthread_create(&tid, NULL, handle_client, pid_ptr) == 0) {
                    pthread_detach(tid);
                    procesados[procesados_count++] = pid;
                } else {
                    free(pid_ptr);
                    perror("pthread_create");
                }
            }
        }
        sleep(1);
    }

    munmap(reg, sizeof(ClientRegistry));
    close(shm_fd);
    return 0;
}
