#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include "../include/data.h"

char CURRENT_USER[50] = "";

int register_user(const char *username, const char *password) {
    FILE *f = fopen("data/users.txt", "a");
    if (!f) return 0;
    fprintf(f, "%s,%s\n", username, password);
    fclose(f);
    printf("Usuario registrado con éxito\n");
    return 1;
}

int login_user(const char *username, const char *password) {
    FILE *f = fopen("data/users.txt", "r");
    if (!f) return 0;

    char line[100], u[50], p[50];
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "%[^,],%s", u, p);
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
            strcpy(CURRENT_USER, username);
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void hide_password(char *buffer, size_t max_length) {
    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    fgets(buffer, max_length, stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

void search_song_by_input() {
    char query[100];
    printf("🔍 Enter song title or artist to search: ");
    fgets(query, sizeof(query), stdin);
    query[strcspn(query, "\n")] = 0;

    FILE *file = fopen("data/songs.txt", "r");
    if (!file) {
        printf("❌ Could not open songs file.\n");
        return;
    }

    char line[256];
    int found = 0;
    printf("\n📋 Search results:\n");

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, query) != NULL) {
            printf("🎶 %s", line);
            found = 1;
        }
    }

    if (!found) {
        printf("No matches found for '%s'.\n", query);
    }

    fclose(file);
}
