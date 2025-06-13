#include <openssl/sha.h> // Asegúrate de que esté incluido para usar SHA256
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

<<<<<<< Updated upstream
#define LOGIN_INPUT_MAX 64
#define MAX_SONGS 50
#define NUM_OPTIONS 4
void hash_password(const char *password, char *output_hex) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)password, strlen(password), hash);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_hex + (i * 2), "%02x", hash[i]);
    }
    output_hex[64] = '\0'; // SHA256 produce 64 caracteres hexadecimales
=======
void register_user(const char *username, const char *password) {
    char hashed_password[HASH_STRING_LENGTH];
    hash_sha256(password, hashed_password);

    FILE *file = fopen("data/users.txt", "a");
    if (!file) {
        printf("Errors opening users\n");
        return;
    }

    fprintf(file, "%s:%s\n", username, hashed_password);
    fclose(file);

    printf("User succesfully registered\n");
>>>>>>> Stashed changes
}

void register_interface(char *username, char *password, SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    clean_screen();
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    keypad(stdscr, TRUE);

    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);
    bkgd(COLOR_PAIR(1));
    refresh();

<<<<<<< Updated upstream
    int exit_flag = 0;
    do {
        clear();
        mvprintw(y, x, "Register New User");
        mvprintw(y + 2, x, "New username (cannot be empty): ");
        mvprintw(y + 8, x, "Press ESC to exit");
        echo();
        move(y + 2, x + 31);

        int ch, i = 0;
        while ((ch = getch()) != '\n' && i < LOGIN_INPUT_MAX - 1) {
            if (ch == 27) {
                exit_flag = 1;
                break;
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (i > 0) {
                    i--;
                    mvaddch(y + 2, x + 31 + i, ' ');
                    move(y + 2, x + 31 + i);
                }
            } else if (ch >= 32 && ch <= 126) {
                username[i++] = ch;
                mvaddch(y + 2, x + 31 + i - 1, ch);
            }
        }
        username[i] = '\0';
        noecho();
        if (exit_flag) break;
    } while (strlen(username) == 0);

    if (!exit_flag) {
        int valid_password = 0;
        while (!valid_password) {
            memset(password, 0, LOGIN_INPUT_MAX);
            mvprintw(y + 4, x, "                                   ");
            mvprintw(y + 4, x, "New password (cannot be empty): ");
            move(y + 4, x + 31);

            int i = 0, ch;
            while ((ch = getch()) != '\n' && i < LOGIN_INPUT_MAX - 1) {
                if (ch == 27) {
                    exit_flag = 1;
                    break;
                } else if (ch == KEY_BACKSPACE || ch == 127) {
                    if (i > 0) {
                        i--;
                        mvaddch(y + 4, x + 31 + i, ' ');
                        move(y + 4, x + 31 + i);
                    }
                } else if (ch >= 32 && ch <= 126) {
                    password[i++] = ch;
                    mvaddch(y + 4, x + 31 + i - 1, '*');
                }
            }
            password[i] = '\0';
            if (exit_flag) break;

            move(y + 6, x);
            clrtoeol();
            if (strlen(password) == 0) {
                mvprintw(y + 6, x, "Password cannot be empty. Try again.");
            } else {
                valid_password = 1;
            }
        }
    }

    if (!exit_flag) {
        mvprintw(y + 6, x, "Registering...");
        refresh();
        sleep(1);
=======
    int y = 4, x = 6;
    int w = 60, h = 10;
    int startx = (COLS - w) / 2;
    int starty = (LINES - h) / 2;

    WINDOW *win = newwin(h, w, starty, startx);
    wbkgd(win, COLOR_PAIR(1));
    box(win, 0, 0);
    wattron(win, A_BOLD);
    mvwprintw(win, 1, 2, "Register New User");
    wattroff(win, A_BOLD);
    mvwprintw(win, 3, 2, "Username: ");
    wrefresh(win);

    move(starty + 3, startx + 21);
    echo();
    getnstr(username, LOGIN_INPUT_MAX - 1);
    noecho();

    mvwprintw(win, 5, 2, "Password: ");
    wrefresh(win);
    move(starty + 5, startx + 13);

    int i = 0;
    int ch;
    while ((ch = getch()) != '\n' && i < LOGIN_INPUT_MAX - 1) {
        if (ch >= 32 && ch <= 126) {
            password[i++] = (char)ch;
            mvaddch(starty + 5, startx + 13 + i - 1, '*');
        }
>>>>>>> Stashed changes
    }

    delwin(win);
    clear();
    endwin();

    if (!exit_flag) {
        // Cifrado con SHA256
        char hashed[65];
        hash_password(password, hashed);

        snprintf(data->message, MAX_MSG, "REGISTER|%s|%s", username, hashed);
        sem_post(client_sem);
        sem_wait(server_sem);

        if (strncmp(data->message, "OK", 2) == 0) {
            printf("User registered successfully.\n");
        } else {
            printf("Error registering user: %s\n", data->message);
        }

        printf("Press Enter to return to menu...Please");
        getchar();
    }

}
