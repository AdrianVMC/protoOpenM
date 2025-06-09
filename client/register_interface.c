#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "../include/hash_utils.h"
#include "../include/shared.h"
#include "../include/client_registry.h"

void register_user(const char *username, const char *password) {
    char hashed_password[HASH_STRING_LENGTH];
    hash_sha256(password, hashed_password);

    FILE *file = fopen("data/users.txt", "a");
    if (!file) {
        printf("Error al abrir el archivo de usuarios\n");
        return;
    }

    fprintf(file, "%s:%s\n", username, hashed_password);
    fclose(file);

    printf("Usuario registrado correctamente.\n");
}

void register_user_interface(char *username, char *password) {
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    keypad(stdscr, TRUE);

    int y = 5, x = 10;

    mvprintw(y, x, "Register New User");
    mvprintw(y + 2, x, "Username (cannot be empty): ");
    move(y + 2, x + 31);

    int i = 0;
    while ((i = getch()) != '\n' && i < LOGIN_INPUT_MAX) {
        if (i >= 32 && i <= 126) {
            username[i++] = (char)i;
            mvaddch(y + 2, x + 31 + i - 1, (char)i);
        }
    }
    username[i] = '\0';

    mvprintw(y + 4, x, "New password (cannot be empty): ");
    move(y + 4, x + 31);

    i = 0;
    while ((i = getch()) != '\n' && i < LOGIN_INPUT_MAX) {
        if (i >= 32 && i <= 126) {
            password[i++] = (char)i;
            mvaddch(y + 4, x + 31 + i - 1, '*');
        }
    }
    password[i] = '\0';

    register_user(username, password);

    clear();
    endwin();
}
