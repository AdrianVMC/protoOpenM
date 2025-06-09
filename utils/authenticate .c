#include <stdio.h>
#include <string.h>
#include "../include/hash_utils.h"
int authenticate(const char *username, const char *password) {
    // Abrir el archivo de usuarios
    FILE *file = fopen("data/users.txt", "r");
    if (!file) {
        printf("Error al abrir el archivo de usuarios\n");
        return 0; // Fallo en la apertura del archivo
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // Eliminar el salto de línea
        char *file_user = strtok(line, ":"); // Obtener nombre de usuario
        char *file_pass = strtok(NULL, ":"); // Obtener la contraseña cifrada

        // Si el nombre de usuario coincide, comparar las contraseñas
        if (file_user && file_pass && strcmp(file_user, username) == 0) {
            // Comparar el hash de la contraseña ingresada con el hash almacenado
            if (compare_hashes(password, file_pass)) {
                fclose(file);
                return 1; // Usuario autenticado correctamente
            } else {
                fclose(file);
                return 0; // Contraseña incorrecta
            }
        }
    }

    fclose(file);
    return 0; // Usuario no encontrado
}
