#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "../include/shared.h"
#include "../include/shared_utils.h"
#include "../include/client_registry.h"

#define LOGIN_INPUT_MAX 64
#define MAX_SONGS 50
#define NUM_OPTIONS 3

const char *menu_options[NUM_OPTIONS] = {
    "View songs",
    "Search song",
    "Logout"
};

void clean_screen() {
    printf("\033[H\033[J");
}


int show_main_menu_ncurses() {
    clean_screen();
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
        mvprintw(1, 2, "Main Menu");

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
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % NUM_OPTIONS;
                break;
            case '\n': {
                selected = highlight;
                endwin();
                return selected;
            }
            case 27:
                endwin();
                return -1;
        }
    }
}


void login_interface(char *username, char *password) {
    clean_screen();
    initscr();
    noecho();
    cbreak();

    int y = 5, x = 10;
    mvprintw(y, x, "Login System");
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


void view_songs(SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    clean_screen();
    snprintf(data->message, MAX_MSG, "SONGS");
    sem_post(client_sem);
    sem_wait(server_sem);

    if (strncmp(data->message, "SONGS|", 6) == 0) {
        char *list = data->message + 6;
        char *song = strtok(list, ";");
        int index = 1;
        printf("\nAvailable songs:\n");
        while (song) {
            printf("  %2d. %s\n", index++, song);
            song = strtok(NULL, ";");
        }
    } else {
        printf("%s\n", data->message);
    }

    printf("\nPress ENTER to return to the menu...");
    getchar();
}


void search_song(SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    clean_screen();
    char search_term[64];
    printf("\nEnter the name or part of the song title: ");
    fgets(search_term, sizeof(search_term), stdin);
    search_term[strcspn(search_term, "\n")] = '\0';

    snprintf(data->message, MAX_MSG, "SEARCH|%s", search_term);
    sem_post(client_sem);
    sem_wait(server_sem);

    if (strncmp(data->message, "FOUND|", 6) == 0) {
        char *list = data->message + 6;
        char *song = strtok(list, ";");
        int index = 1;
        printf("\nSearch results:\n");
        while (song) {
            printf("  %2d. %s\n", index++, song);
            song = strtok(NULL, ";");
        }
    } else {
        printf("%s\n", data->message);
    }

    printf("\nPress ENTER to return to the menu...");
    getchar();
}


int main() {
    char username[LOGIN_INPUT_MAX];
    char password[LOGIN_INPUT_MAX];
    pid_t pid = getpid();

    char shm_name[64], sem_client[64], sem_server[64];
    generate_names(shm_name, sem_client, sem_server, pid);

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    SharedData *data = mmap(0, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    sem_t *client_sem = sem_open(sem_client, O_CREAT, 0666, 0);
    sem_t *server_sem = sem_open(sem_server, O_CREAT, 0666, 0);

    register_client(pid);

    login_interface(username, password);
    snprintf(data->message, MAX_MSG, "LOGIN|%s|%s", username, password);
    sem_post(client_sem);
    sem_wait(server_sem);

    if (strcmp(data->message, "OK") == 0) {
        printf("\n️Welcome, %s\n", username);

        int exit_program = 0;
        while (!exit_program) {
            int option = show_main_menu_ncurses();

            switch (option) {
                case 0:
                    view_songs(data, client_sem, server_sem);
                    break;
                case 1:
                    search_song(data, client_sem, server_sem);
                    break;
                case 2:
                    snprintf(data->message, MAX_MSG, "LOGOUT");
                    sem_post(client_sem);
                    exit_program = 1;
                    break;
                default:
                    printf("Invalid option\n");
            }
        }
    } else {
        printf("\nAuthentication failed.\n");
    }

    munmap(data, sizeof(SharedData));
    close(shm_fd);
    sem_close(client_sem);
    sem_close(server_sem);
    shm_unlink(shm_name);
    sem_unlink(sem_client);
    sem_unlink(sem_server);

    return 0;
}
