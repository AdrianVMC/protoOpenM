#include <stdio.h>
#include <string.h>
#include "../include/data.h"

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
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void list_songs() {
    FILE *f = fopen("data/songs.txt", "r");
    if (!f) {
        printf("No se pudo abrir el archivo de canciones.\n");
        return;
    }

    char line[200], title[100], artist[100];
    printf("=== Canciones Disponibles ===\n");
    while (fgets(line, sizeof(line), f)) {
        sscanf(line, "%[^,],%[^\n]", title, artist);
        printf("- %s por %s\n", title, artist);
    }

    fclose(f);
}
