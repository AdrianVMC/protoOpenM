// server/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <../include/data.h>

#define SHARED_FILE "users.txt"
#define POLL_INTERVAL 1  // segundos

int main() {
    char last_user[100] = "";

    printf("🟢 Servidor esperando usuarios...\n");

    while (1) {
        FILE* file = fopen(SHARED_FILE, "r");
        if (file) {
            char current_user[100] = "";
            fgets(current_user, sizeof(current_user), file);
            fclose(file);

            // Si hay un nuevo usuario y no es igual al anterior
            if (strlen(current_user) > 0 && strcmp(current_user, last_user) != 0) {
                current_user[strcspn(current_user, "\n")] = 0;  // quitar salto de línea
                printf("✅ Usuario conectado: %s\n", current_user);
                strcpy(last_user, current_user);
            }
        }

        sleep(POLL_INTERVAL); // Esperar antes de revisar otra vez
    }

    return 0;
}
