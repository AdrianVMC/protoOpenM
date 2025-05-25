#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include "../include/data.h"

char CURRENT_USER[50] = "";


void play_song() {
    char title[100];
    printf("Ingrese el nombre exacto de la canción a reproducir: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = 0;

    FILE *file = fopen("data/songs.txt", "r");
    if (!file) {
        printf("No se pudo abrir el archivo de canciones.\n");
        return;
    }

    char line[256];
    char song[100], artist[100], path[200];
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%[^,],%[^\n]", song, artist, path);
        if (strcasecmp(title, song) == 0) {
            found = 1;
            printf("Reproduciendo: %s - %s\n", song, artist);

            FILE *audio_file = fopen(path, "r");
            if (!audio_file) {
                printf("El archivo de audio no fue encontrado: %s\n", path);
                break;
            }
            fclose(audio_file);
            //Puede haber problemas de compatibilidad con MAC y Windows
            char command[300];
            snprintf(command, sizeof(command), "afplay \"%s\"", path);
            system(command);
            break;
        }
    }

    if (!found) {
        printf("No se encontró una canción con ese título.\n");
    }

    fclose(file);
}


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
    printf("Ingrese el título o artista a buscar: ");
    fgets(query, sizeof(query), stdin);
    query[strcspn(query, "\n")] = 0;

    FILE *file = fopen("data/songs.txt", "r");
    if (!file) {
        printf("No se pudo abrir el archivo de canciones.\n");
        return;
    }

    char line[256];
    char song[100], artist[100];
    int found = 0;

    printf("\nResultados de la búsqueda:\n");

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%[^,],%*s", song, artist);

        if (strstr(song, query) != NULL || strstr(artist, query) != NULL) {
            printf("Título: %s | Artista: %s\n", song, artist);
            found = 1;
        }
    }

    if (!found) {
        printf("No se encontraron coincidencias para '%s'.\n", query);
    }

    fclose(file);
}

