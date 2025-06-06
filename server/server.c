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

int authenticate(const char *username, const char *password);
void handle_songs_request(SharedData *data);
void handle_search_request(SharedData *data);
void handle_get_request(SharedData *data, sem_t *client_sem, sem_t *server_sem);
void unregister_client(pid_t pid);
void *handle_client(void *arg);
void cleanup(int sig);

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

void handle_songs_request(SharedData *data) {
    FILE *file = fopen("data/songs.txt", "r");
    if (!file) {
        snprintf(data->message, MAX_MSG, "ERROR|Could not open songs.txt");
        return;
    }

    char buffer[MAX_MSG] = "SONGS|";
    char line[128];
    int first = 1;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        char *title = strtok(line, ":");
        if (!first) strncat(buffer, ";", MAX_MSG - strlen(buffer) - 1);
        strncat(buffer, title, MAX_MSG - strlen(buffer) - 1);
        first = 0;
    }

    fclose(file);
    strncpy(data->message, buffer, MAX_MSG);
}

void handle_search_request(SharedData *data) {
    char *term = data->message + 7;
    FILE *file = fopen("data/songs.txt", "r");

    if (!file) {
        snprintf(data->message, MAX_MSG, "ERROR|Cannot open songs.txt");
        return;
    }

    char result[MAX_MSG] = "FOUND|";
    char line[128];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        if (strstr(line, term)) {
            if (found) strncat(result, ";", MAX_MSG - strlen(result) - 1);
            char *title = strtok(line, ":");
            strncat(result, title, MAX_MSG - strlen(result) - 1);
            found = 1;
        }
    }

    fclose(file);
    if (found) {
        strncpy(data->message, result, MAX_MSG);
    } else {
        snprintf(data->message, MAX_MSG, "NOT_FOUND|No matches found");
    }
}

void handle_get_request(SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    char *requested_title = data->message + 4;

    FILE *file = fopen("data/songs.txt", "r");
    if (!file) {
        snprintf(data->message, MAX_MSG, "ERROR|songs.txt not found");
        sem_post(server_sem);
        return;
    }

    char line[256];
    char filepath[256] = "";
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        char *title = strtok(line, ":");
        strtok(NULL, ":"); // Skip artist
        char *path = strtok(NULL, ":");

        if (title && path && strcmp(title, requested_title) == 0) {
            strncpy(filepath, path, sizeof(filepath));
            break;
        }
    }
    fclose(file);

    if (strlen(filepath) == 0) {
        snprintf(data->message, MAX_MSG, "ERROR|Song not found");
        sem_post(server_sem);
        return;
    }

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        snprintf(data->message, MAX_MSG, "ERROR|File could not be opened");
        sem_post(server_sem);
        return;
    }

    while (1) {
        int bytes = fread(data->audio_chunk, 1, AUDIO_CHUNK_SIZE, fp);
        data->chunk_size = bytes;
        data->is_last_chunk = feof(fp) ? 1 : 0;

        sem_post(server_sem);
        sem_wait(client_sem);

        if (data->is_last_chunk) break;
    }

    fclose(fp);
}

void unregister_client(pid_t pid) {
    sem_t *reg_sem = sem_open(REGISTRY_SEM_NAME, 0);
    if (reg_sem == SEM_FAILED) return;

    sem_wait(reg_sem);
    int shm_fd = shm_open(REGISTRY_SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        sem_post(reg_sem);
        sem_close(reg_sem);
        return;
    }

    ClientRegistry *reg = mmap(NULL, sizeof(ClientRegistry), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (reg == MAP_FAILED) {
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
            printf("Client PID %d removed from registry\n", pid);
            break;
        }
    }

    munmap(reg, sizeof(ClientRegistry));
    close(shm_fd);
    sem_post(reg_sem);
    sem_close(reg_sem);
}

void *handle_client(void *arg) {
    pid_t pid = *(pid_t *)arg;
    free(arg);

    char shm_name[64], sem_client[64], sem_server[64];
    generate_names(shm_name, sem_client, sem_server, pid);

    int shm_fd = -1;
    for (int i = 0; i < 20; i++) {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (shm_fd != -1) break;
        usleep(500000);
    }

    if (shm_fd == -1) {
        fprintf(stderr, "Cannot open shared memory for PID %d\n", pid);
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

    printf("Attending PID client: %d\n", pid);

    while (1) {
        if (sem_wait(client_sem) == -1) break;

        if (strncmp(data->message, "LOGIN|", 6) == 0) {
            char *username = strtok(data->message + 6, "|");
            char *password = strtok(NULL, "|");
            if (username && password && authenticate(username, password)) {
                snprintf(data->message, MAX_MSG, "OK");
                printf("Verified client: %s\n", username);
            } else {
                snprintf(data->message, MAX_MSG, "ERROR");
                printf("Invalid password or user: %s\n", username ? username : "NULL");
            }
        }
        else if (strncmp(data->message, "LOGOUT", 6) == 0) {
            printf("Client PID %d logout\n", pid);
            unregister_client(pid);
            break;
        }
        else if (strncmp(data->message, "SONGS", 5) == 0) {
            handle_songs_request(data);
        }
        else if (strncmp(data->message, "SEARCH|", 7) == 0) {
            handle_search_request(data);
        }
        else if (strncmp(data->message, "GET|", 4) == 0) {
            handle_get_request(data, client_sem, server_sem);
            continue;
        }

        sem_post(server_sem);
    }

    munmap(data, sizeof(SharedData));
    close(shm_fd);
    sem_close(client_sem);
    sem_close(server_sem);
    pthread_exit(NULL);
}

void cleanup(int sig) {
    printf("\nCleaning resources...\n");
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
    printf("Initialized Server. Waiting for clients...\n");

    pid_t handled[MAX_CLIENTS] = {0};
    int handled_count = 0;

    while (1) {
        for (int i = 0; i < reg->count; i++) {
            pid_t pid = reg->pids[i];
            int known = 0;

            for (int j = 0; j < handled_count; j++) {
                if (handled[j] == pid) {
                    known = 1;
                    break;
                }
            }

            if (!known) {
                pid_t *pid_ptr = malloc(sizeof(pid_t));
                *pid_ptr = pid;
                pthread_t tid;

                if (pthread_create(&tid, NULL, handle_client, pid_ptr) == 0) {
                    pthread_detach(tid);
                    handled[handled_count++] = pid;
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
