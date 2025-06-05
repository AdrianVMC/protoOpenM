#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "../include/shared.h"
#include "../include/shared_utils.h"
#include "../include/client_registry.h"

#define LOGIN_INPUT_MAX 64
#define MAX_CANCIONES 50
#define NUM_OPTIONS 3

const char *menu_options[NUM_OPTIONS] = {
    "Ver canciones",
    "Buscar cancion",
    "Cerrar sesion"
};

int show_main_menu_ncurses() {
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    int highlight = 0;
    int input;
    int selected = -1;

    while (1) {
        clear();
        mvprintw(1, 2, "🎵 Menu Principal");

        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (i == highlight) {
                attron(A_REVERSE);
                mvprintw(3 + i, 4, "> %s", menu_options[i]);
                attroff(A_REVERSE);
            } else {
                mvprintw(3 + i, 6, "  %s", menu_options[i]);
            }
        }

        refresh();
        input = getch();
        switch (input) {
            case KEY_UP:
                highlight = (highlight == 0) ? NUM_OPTIONS - 1 : highlight - 1;
                clear();
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % NUM_OPTIONS;
                clear();
                break;
            case KEY_LEFT:
                highlight = (highlight + 1) % NUM_OPTIONS;
                clear();
            break;
            case '\n': {
                selected = highlight;
                clear();
                endwin();
                return selected;
            }
            case 27:
                clear();
                endwin();
                return -1;
        }
    }
}

void show_songs_ncurses(char **canciones, int total) {
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    int y = 2;
    mvprintw(0, 2, "🎵 Canciones disponibles:");
    for (int i = 0; i < total; i++) {
        mvprintw(y++, 4, "%2d. %s", i + 1, canciones[i]);
    }
    mvprintw(y + 1, 2, "Presiona ENTER para volver...");
    refresh();
    while (getch() != '\n');
    clear();
    endwin();
}

void login_interface(char *username, char *password) {
    initscr();
    noecho();
    cbreak();

    int y = 5, x = 10;
    mvprintw(y, x, "🔐 Login System");
    mvprintw(y + 2, x, "Username: ");
    echo();
    getnstr(username, LOGIN_INPUT_MAX);
    noecho();

    mvprintw(y + 4, x, "Password: ");
    echo();
    getnstr(password, LOGIN_INPUT_MAX);
    noecho();

    mvprintw(y + 6, x, "Authenticating...");
    refresh();
    sleep(1);
    clear();
    endwin();
}

int main() {
    char username[LOGIN_INPUT_MAX];
    char password[LOGIN_INPUT_MAX];
    pid_t pid = getpid();

    char shm_name[64], sem_client[64], sem_server[64];
    generate_names(shm_name, sem_client, sem_server, pid);

    // Crear memoria compartida y semáforos antes de registrar cliente
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    SharedData *data = mmap(0, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    sem_t *client_sem = sem_open(sem_client, O_CREAT, 0666, 0);
    sem_t *server_sem = sem_open(sem_server, O_CREAT, 0666, 0);

    // Registrar cliente
    register_client(pid);

    // Login
    login_interface(username, password);
    snprintf(data->message, MAX_MSG, "LOGIN|%s|%s", username, password);
    sem_post(client_sem);
    sem_wait(server_sem);

    if (strcmp(data->message, "OK") == 0) {

        int salir = 0;
        while (!salir) {
            int opcion = show_main_menu_ncurses();  // 0: canciones, 1: logout

            switch (opcion) {
                case 0: {
                    // Solicitar canciones al servidor
                    snprintf(data->message, MAX_MSG, "SONGS");
                    sem_post(client_sem);
                    sem_wait(server_sem);

                    if (strncmp(data->message, "SONGS|", 6) == 0) {
                        char *lista = data->message + 6;
                        char *cancion = strtok(lista, ";");
                        char *canciones[MAX_CANCIONES];
                        int total = 0;

                        while (cancion && total < MAX_CANCIONES) {
                            canciones[total++] = strdup(cancion);
                            cancion = strtok(NULL, ";");
                        }

                        show_songs_ncurses(canciones, total);

                        for (int i = 0; i < total; i++) {
                            free(canciones[i]);
                        }
                    } else {
                        printf("⚠️ %s\n", data->message);
                    }
                    break;
                }
                case 1:
                    snprintf(data->message, MAX_MSG, "LOGOUT");
                    sem_post(client_sem);
                    salir = 1;
                    break;
                default:
                    printf("❌ Opción inválida\n");
            }
        }
    } else {
        printf("\n❌ Error de autenticación.\n");
    }

    // Limpieza
    munmap(data, sizeof(SharedData));
    close(shm_fd);
    sem_close(client_sem);
    sem_close(server_sem);
    shm_unlink(shm_name);
    sem_unlink(sem_client);
    sem_unlink(sem_server);

    return 0;
}
