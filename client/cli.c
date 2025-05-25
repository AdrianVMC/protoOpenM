#include <stdio.h>
#include <string.h>
#include "../include/cli.h"
#include "../include/data.h"

void show_main_menu() {
    printf("=== OpenMS ===\n");
    printf("1. Registrarse\n");
    printf("2. Iniciar sesión\n");
    printf("3. Ver canciones\n");
    printf("4. Cerrar Sesión\n");
    printf("0. Salir\n");
    printf("Seleccione una opción: ");
}

void handle_main_menu() {
    int option;
    char username[50], password[50];

    do {
        show_main_menu();
        scanf("%d", &option);
        getchar();

        switch (option) {
            case 1:
                printf("Usuario: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = 0;

                printf("Contraseña: ");
                hide_password(password, sizeof(password));

                register_user(username, password);
                break;

            case 2:
                printf("Usuario: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = 0;

                printf("Contraseña: ");
                hide_password(password, sizeof(password));

                if (login_user(username, password)) {
                    printf("Inicio de sesión exitoso\n");
                } else {
                    printf("Error al iniciar sesión\n");
                }
                break;

            case 3:
                list_songs();
                break;

            case 4:
                strcpy(CURRENT_USER, "");
                printf("Sesión cerrada.\n");
                break;

            }
    } while (option != 0);
}

int main() {
    handle_main_menu();
    return 0;
}