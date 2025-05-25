#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include "../include/data.h"


#define LENGTH_BAR 40
char CURRENT_USER[50] = "";

void draw_progress_bar(int progress, int duration, const char *statem) {
    int fstate = (progress * LENGTH_BAR) / duration;
    mvprintw(5, 2, "[");
    for (int i = 0; i < LENGTH_BAR; i++) {
        if (i < fstate) {
            printw("=");
        } else {
            printw(" ");
        }
    }
    printw("] %d%%", (progress * 100) / duration);
    mvprintw(3, 2, "Estado: %s", statem);
    refresh();
}

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

    char line[300];
    char song[100], artist[100], path[200];
    int duration = 0;
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%[^,],%[^,],%d", song, artist, path, &duration);
        if (strcasecmp(title, song) == 0) {
            found = 1;

            initscr();
            noecho();
            cbreak();
            timeout(100);
            keypad(stdscr, TRUE);

            pid_t pid = fork();
            if (pid == 0) {
                execlp("afplay", "afplay", path, NULL);
                perror("Error al reproducir audio");
                exit(1);
            }

            char statem[20] = "Reproduciendo";
            int progress = 0;
            int pausado = 0;

            while (progress <= duration) {
                clear();
                mvprintw(1, 2, "🎵 Reproduciendo: %s - %s", song, artist);
                mvprintw(7, 2, "Controles: [p] Pausar | [r] Reanudar | [s] Detener");

                draw_progress_bar(progress, duration, statem);

                int ch = getch();
                if (ch == 'p') {
                    kill(pid, SIGSTOP);
                    strcpy(statem, "Pausado");
                    pausado = 1;
                } else if (ch == 'r') {
                    kill(pid, SIGCONT);
                    strcpy(statem, "Reproduciendo");
                    pausado = 0;
                } else if (ch == 's') {
                    kill(pid, SIGKILL);
                    strcpy(statem, "Detenido");
                    break;
                }

                if (!pausado) {
                    sleep(1);
                    progress++;
                }
            }

            endwin();
            wait(NULL);
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

