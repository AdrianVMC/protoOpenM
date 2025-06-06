#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "../include/shared.h"
#include "../include/shared_utils.h"
#include "../include/client_registry.h"

#define LOGIN_INPUT_MAX 64
#define MAX_SONGS 50
#define NUM_OPTIONS 4

const char *menu_options[NUM_OPTIONS] = {
    "View songs",
    "Search song",
    "Reproduce song",
    "Logout"
};

void clean_screen() {
    printf("\033[H\033[J");
}

int get_mp3_duration(const char *filename) {
    char command[256];
    snprintf(command, sizeof(command),
             "ffprobe -v error -show_entries format=duration -of csv=p=0 \"%s\"", filename);

    FILE *fp = popen(command, "r");
    if (!fp) return 30;

    char buffer[64];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        pclose(fp);
        return 30;
    }

    pclose(fp);
    return (int)(atof(buffer) + 0.5);
}

void show_progress_bar(int duration, pid_t player_pid) {
    int paused = 0;
    int elapsed = 0;
    time_t start = time(NULL);
    time_t pause_start = 0;
    int total_paused_time = 0;

    nodelay(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    while (elapsed < duration) {
        int ch = getch();

        switch (ch) {
            case 'p':
            case 'P':
                if (!paused) {
                    kill(player_pid, SIGSTOP);
                    paused = 1;
                    pause_start = time(NULL);
                }
            break;
            case 'r':
            case 'R':
                if (paused) {
                    kill(player_pid, SIGCONT);
                    paused = 0;
                    total_paused_time += (int)(time(NULL) - pause_start);
                }
            break;
            case 'q':
            case 'Q':
                kill(player_pid, SIGTERM);
            waitpid(player_pid, NULL, 0);
            nodelay(stdscr, FALSE);
            curs_set(1);
            return;
        }

        if (!paused) {
            time_t now = time(NULL);
            elapsed = (int)(now - start - total_paused_time);
        }

        clear();
        mvprintw(2, 2, "Playing song...");
        mvprintw(3, 2, "Status: %s", paused ? "Paused" : "Playing");
        mvprintw(5, 2, "Controls: [P]ause | [R]esume | [Q]uit");

        mvprintw(7, 2, "[");
        int bar_width = 50;
        int pos = (elapsed * bar_width) / duration;
        if (pos > bar_width) pos = bar_width;

        for (int i = 0; i < bar_width; i++) {
            printw(i < pos ? "=" : " ");
        }
        printw("] %d/%d sec", elapsed, duration);
        refresh();
    }

    nodelay(stdscr, FALSE);
    curs_set(1);
    waitpid(player_pid, NULL, 0);
}




void prompt_song_name(char *buffer, size_t size) {
    clean_screen();
    printf("Enter song's exact name: ");
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
}

void download_and_play_song(SharedData *data, sem_t *client_sem, sem_t *server_sem, const char *song_name) {
    clean_screen();
    snprintf(data->message, MAX_MSG, "GET|%s", song_name);
    sem_post(client_sem);

    char temp_filename[128];
    snprintf(temp_filename, sizeof(temp_filename), "/tmp/%s_temp.mp3", song_name);
    FILE *fp = fopen(temp_filename, "wb");

    while (1) {
        sem_wait(server_sem);
        fwrite(data->audio_chunk, 1, data->chunk_size, fp);
        sem_post(client_sem);
        if (data->is_last_chunk) break;
    }

    fclose(fp);

    initscr();
    noecho();
    cbreak();

    pid_t pid = fork();
    if (pid == 0) {
        execlp("mpg123", "mpg123", "--quiet", temp_filename, NULL);
        perror("Error al ejecutar mpg123");
        exit(1);
    } else {
        int duration = get_mp3_duration(temp_filename);
        show_progress_bar(duration, pid);
    }

    endwin();
    remove(temp_filename);
    clean_screen();
}

void play_song_by_name(SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    clean_screen();
    char song_name[64];
    prompt_song_name(song_name, sizeof(song_name));

    if (strlen(song_name) == 0) {
        printf("No name given.\nPress Enter to go back to menu...");
        getchar();
        return;
    }

    download_and_play_song(data, client_sem, server_sem, song_name);
    printf("\nReproduction Finalized. Press Enter to get back to menu...");
    getchar();
}


void login_interface(char *username, char *password) {
    clean_screen();
    initscr();
    cbreak();
    noecho();
    curs_set(1);

    int y = 5, x = 10;

    // === Obtener username (no vacío) ===
    do {
        clear();
        mvprintw(y, x, "Login System");
        mvprintw(y + 2, x, "Username (cannot be empty): ");
        echo();
        move(y + 2, x + 29); // Mover cursor justo después del mensaje
        getnstr(username, LOGIN_INPUT_MAX - 1);
        noecho();
    } while (strlen(username) == 0);

    // === Obtener contraseña (no vacía, oculta con '*') ===
    int valid_password = 0;
    while (!valid_password) {
        memset(password, 0, LOGIN_INPUT_MAX); // Limpiar buffer
        mvprintw(y + 4, x, "Password (cannot be empty): ");
        move(y + 4, x + 29);

        int i = 0, ch;
        while ((ch = getch()) != '\n' && i < LOGIN_INPUT_MAX - 1) {
            if (ch == KEY_BACKSPACE || ch == 127) {
                if (i > 0) {
                    i--;
                    mvaddch(y + 4, x + 29 + i, ' ');
                    move(y + 4, x + 29 + i);
                }
            } else if (ch >= 32 && ch <= 126) { // Caracteres imprimibles
                password[i++] = ch;
                mvaddch(y + 4, x + 29 + i - 1, '*');
            }
        }
        password[i] = '\0';

        // Validar
        move(y + 6, x);
        clrtoeol();  // Limpiar mensaje previo
        if (strlen(password) == 0) {
            mvprintw(y + 6, x, "Password cannot be empty. Try again.");
        } else {
            valid_password = 1;
        }
    }

    // Confirmación
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

    clean_screen();
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

    printf("\nPress Enter to return to the menu ...");
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
            case '\n':
                selected = highlight;
            endwin();
            return selected;
            case 27:
                endwin();
            return -1;
        }
    }
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
                    play_song_by_name(data, client_sem, server_sem);
                    break;
                case 3:
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
