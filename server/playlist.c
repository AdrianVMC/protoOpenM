/* server/playlist.c  –  Manejo de playlists por usuario */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include "../include/shared.h"
#include "playlist.h"

/* Ruta:  data/playlists/<usuario>.txt */
static void pl_path(char *buf, size_t sz, const char *u)
{
    snprintf(buf, sz, "%s/playlists/%s.txt", DATADIR, u);
}

/* ------------------------------------------------------------------ */
/*  playlist_handle: procesa PLIST|SHOW / ADD / DEL / PLAY            */
/* ------------------------------------------------------------------ */
void playlist_handle(SharedData *d,
                     const char *user,
                     sem_t *client_sem,   /* no se usan aquí pero se        */
                     sem_t *server_sem)   /* mantienen la firma uniforme    */
{
    (void)client_sem; (void)server_sem;   /* evita warnings de ‘unused’     */

    char *cmd = strtok(d->message + 6, "|");
    char *arg = strtok(NULL, "|");

    /* Archivo playlists del usuario */
    char file[256];
    pl_path(file, sizeof file, user);

    /* ---------- SHOW / PLAY ---------- */
    if (!strcmp(cmd, "SHOW") || !strcmp(cmd, "PLAY")) {
        FILE *fp = fopen(file, "r");
        if (!fp) {                       /* playlist vacía */
            snprintf(d->message, MAX_MSG, "EMPTY");
            return;
        }
        char res[MAX_MSG] = "PLIST|";
        char line[128];
        int first = 1;
        while (fgets(line, sizeof line, fp)) {
            line[strcspn(line, "\n")] = '\0';
            if (!first) strncat(res, ";", MAX_MSG - strlen(res) - 1);
            strncat(res, line, MAX_MSG - strlen(res) - 1);
            first = 0;
        }
        fclose(fp);
        strncpy(d->message, res, MAX_MSG);
    }

    /* ---------- ADD ---------- */
    else if (!strcmp(cmd, "ADD") && arg) {
        FILE *fp = fopen(file, "a+");
        if (!fp) { snprintf(d->message, MAX_MSG, "ERROR"); return; }

        /* comprobar duplicado */
        int dup = 0; rewind(fp);
        char l[128];
        while (fgets(l, sizeof l, fp)) {
            l[strcspn(l, "\n")] = '\0';
            if (!strcmp(l, arg)) { dup = 1; break; }
        }
        if (!dup) fprintf(fp, "%s\n", arg);
        fclose(fp);
        snprintf(d->message, MAX_MSG, "OK");
    }

    /* ---------- DEL ---------- */
    else if (!strcmp(cmd, "DEL") && arg) {
        FILE *fp = fopen(file, "r");
        if (!fp) { snprintf(d->message, MAX_MSG, "ERROR"); return; }

        /* 4 bytes “.tmp” + '\0'  → +5 bytes al tamaño de file[] */
        char tmp[sizeof(file) + 5];
        snprintf(tmp, sizeof tmp, "%s.tmp", file);

        FILE *out = fopen(tmp, "w");
        char l[128];
        while (fgets(l, sizeof l, fp)) {
            l[strcspn(l, "\n")] = '\0';
            if (strcmp(l, arg)) fprintf(out, "%s\n", l);
        }
        fclose(fp); fclose(out);
        rename(tmp, file);
        snprintf(d->message, MAX_MSG, "OK");
    }

    /* ---------- Comando desconocido ---------- */
    else {
        snprintf(d->message, MAX_MSG, "ERROR");
    }
}
