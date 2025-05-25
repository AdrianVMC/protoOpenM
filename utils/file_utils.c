#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <termios.h>
#include "../include/data.h"

#include "../include/users.h"
#include "../include/songs.h"

char CURRENT_USER[50] = "";

/*-----------------------------------------------------*/
/* REGISTRO Y LOGIN                                    */
/*-----------------------------------------------------*/
int register_user(const char *username, const char *password)
{
    if (users_register(username, password) == 0) {
        printf("Usuario registrado con éxito.\n");
        return 1;
    }
    printf("Error: ese usuario ya existe o no se pudo escribir.\n");
    return 0;
}

int login_user(const char *username, const char *password)
{
    int r = users_login(username, password);
    if (r == 0) {
        strcpy(CURRENT_USER, username);
        return 1;
    }
    if (r == -2) puts("Ya existe una sesión activa con esa cuenta.");
    else         puts("Credenciales incorrectas.");
    return 0;
}

/*-----------------------------------------------------*/
/* UTILIDAD PARA OCULTAR CONTRASEÑA EN TERMINAL        */
/*-----------------------------------------------------*/
void hide_password(char *buffer, size_t max_length)
{
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

/*-----------------------------------------------------*/
/* REPRODUCIR CANCIÓN                                  */
/*-----------------------------------------------------*/
void play_song(void)
{
    char title[100];
    printf("Ingrese el nombre exacto de la canción a reproducir: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = 0;

    Song *arr; size_t n;
    if (songs_load(&arr, &n) <= 0) {
        puts("La biblioteca está vacía o no se pudo leer.");
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        if (strcasecmp(title, arr[i].title) == 0) {
            printf("Reproduciendo: %s - %s\n",
                   arr[i].title, arr[i].artist);

            if (access(arr[i].path, R_OK) != 0) {
                printf("No se encontró el archivo: %s\n", arr[i].path);
                free(arr); return;
            }

#ifdef __APPLE__
            char cmd[256]; snprintf(cmd, sizeof(cmd), "afplay \"%s\"", arr[i].path);
#else
            char cmd[256]; snprintf(cmd, sizeof(cmd), "mpg123 \"%s\"", arr[i].path);
#endif
            system(cmd);
            free(arr); return;
        }
    }
    puts("No se encontró una canción con ese título.");
    free(arr);
}

/*-----------------------------------------------------*/
/* BÚSQUEDA POR TÍTULO O ARTISTA                       */
/*-----------------------------------------------------*/
void search_song_by_input(void)
{
    char query[100];
    printf("Ingrese el título o artista a buscar: ");
    fgets(query, sizeof(query), stdin);
    query[strcspn(query, "\n")] = 0;

    Song *arr; size_t n;
    if (songs_load(&arr, &n) <= 0) {
        puts("No se pudo leer la biblioteca.");
        return;
    }

    int found = 0;
    puts("\nResultados de la búsqueda:");
    for (size_t i = 0; i < n; ++i) {
        if (strcasestr(arr[i].title,  query) ||
            strcasestr(arr[i].artist, query))
        {
            printf("Título: %s | Artista: %s\n",
                   arr[i].title, arr[i].artist);
            found = 1;
        }
    }
    if (!found)
        printf("No se encontraron coincidencias para \"%s\".\n", query);

    free(arr);
}
