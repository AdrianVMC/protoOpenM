#include <stdio.h>
#include <string.h>
#include "../include/hash_utils.h"
int authenticate(const char *username, const char *password)
{
    FILE *file = fopen("data/users.txt", "r");
    if (!file) { perror("users.txt"); return 0; }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        char *file_user  = strtok(line, ":");
        char *file_hash  = strtok(NULL, ":");

        if (file_user && file_hash && strcmp(file_user, username) == 0) {

            char calc[HASH_STRING_LENGTH];
            hash_sha256(password, calc);


            printf("[DEBUG] user=%s\n"
                   "        stored=%s\n"
                   "        calc  =%s\n",
                   username, file_hash, calc);


            fclose(file);
            return strcmp(calc, file_hash) == 0;
        }
    }
    fclose(file);
    return 0;
}
