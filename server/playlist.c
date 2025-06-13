#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <ncurses.h>
#include "../include/shared.h"
#include "../include/playlist.h"
#include "../include/config.h"

static void pl_path(char *buf, size_t sz, const char *u) {
    snprintf(buf, sz, "%s/playlists/%s.txt", DATADIR, u);
}

void playlist_handle(SharedData *d,
                     const char *user,
                     sem_t *client_sem,
                     sem_t *server_sem)
{
    (void)client_sem;
    (void)server_sem;

    char *cmd = strtok(d->message + 6, "|");
    char *arg = strtok(NULL, "|");

    char file[256];
    pl_path(file, sizeof file, user);

    if (!strcmp(cmd, "SHOW") || !strcmp(cmd, "PLAY")) {
        FILE *fp = fopen(file, "r");
        if (!fp) {
            snprintf(d->message, MAX_MSG, "PLIST|\n(empty)");
            return;
        }

        char res[MAX_MSG];
        snprintf(res, sizeof res,
            "PLIST|\n"
            "Playlist de %s\n",
            user);

        char line[128];
        int index = 1;
        while (fgets(line, sizeof line, fp)) {
            line[strcspn(line, "\n")] = '\0';
            char item[140];
            snprintf(item, sizeof item, " %2d. %s\n", index++, line);
            strncat(res, item, MAX_MSG - strlen(res) - 1);
        }
        fclose(fp);
        strncpy(d->message, res, MAX_MSG);
    }

    else if (!strcmp(cmd, "ADD") && arg) {
        FILE *fp = fopen(file, "a+");
        if (!fp) {
            snprintf(d->message, MAX_MSG, "ERROR");
            return;
        }

        int dup = 0;
        rewind(fp);
        char l[128];
        while (fgets(l, sizeof l, fp)) {
            l[strcspn(l, "\n")] = '\0';
            if (!strcmp(l, arg)) {
                dup = 1;
                break;
            }
        }
        if (!dup) fprintf(fp, "%s\n", arg);
        fclose(fp);
        snprintf(d->message, MAX_MSG, "OK");
    }

    else if (!strcmp(cmd, "DEL") && arg) {
        FILE *fp = fopen(file, "r");
        if (!fp) {
            snprintf(d->message, MAX_MSG, "ERROR");
            return;
        }

        char tmp[sizeof(file) + 5];
        snprintf(tmp, sizeof tmp, "%s.tmp", file);

        FILE *out = fopen(tmp, "w");
        char l[128];
        while (fgets(l, sizeof l, fp)) {
            l[strcspn(l, "\n")] = '\0';
            if (strcmp(l, arg)) fprintf(out, "%s\n", l);
        }
        fclose(fp);
        fclose(out);
        rename(tmp, file);
        snprintf(d->message, MAX_MSG, "OK");
    }

    else {
        snprintf(d->message, MAX_MSG, "ERROR");
    }
}
