#include <stdio.h>
#include <string.h>
#include "../include/cli.h"
#include "../include/data.h"

void show_auth_menu() {
    printf("=== Bienvenido a openMS ===\n");
    printf("1. Iniciar sesión\n");
    printf("2. Registrarse\n");
    printf("0. Salir\n");
    printf("Seleccione una opción: ");
}

void show_user_menu() {
    printf("=== Menú de Usuario ===\n");
    printf("1. Buscar canción\n");
    printf("2. Reproducir canción\n");
    printf("3. Detener reproducción\n");
    printf("4. Cerrar sesión\n");
    printf("Seleccione una opción: ");
}

void handle_user_menu() {
    int opt;
    char input[100];

    do {
        show_user_menu();
        scanf("%d", &opt);
        getchar();

        switch (opt) {
            case 1:
                printf("Buscar canción (simulado): ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
                printf("Simulando búsqueda de \"%s\"...\n", input);
                break;

            case 2:
                printf("🎵 Reproduciendo canción (simulado)...\n");
                break;

            case 3:
                printf("⏹ Canción detenida.\n");
                break;

            case 4:
                printf("👋 Cerrando sesión...\n");
                strcpy(CURRENT_USER, "");
                return;

            default:
                printf("Opción inválida.\n");
        }

        printf("\n");

    } while (1);
}

void handle_main_menu() {
    int option;
    char username[50], password[50];

    do {
        show_auth_menu();
        scanf("%d", &option);
        getchar();

        switch (option) {
            case 1:
                printf("Usuario: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = 0;

                printf("Contraseña: ");
                hide_password(password, sizeof(password));

                if (login_user(username, password)) {
                    printf("Inicio de sesión exitoso.\n\n");
                    handle_user_menu();
                } else {
                    printf("Usuario o contraseña incorrectos.\n");
                }
                break;

            case 2:
                printf("Usuario: ");
                fgets(username, sizeof(username), stdin);
                username[strcspn(username, "\n")] = 0;

                printf("Contraseña: ");
                hide_password(password, sizeof(password));

                register_user(username, password);
                break;

            case 0:
                printf("Saliendo de openMS...\n");
                return;

            default:
                printf("Opción inválida.\n");
        }

        printf("\n");

    } while (1);
}


int main() {
    handle_main_menu();
    return 0;
}
