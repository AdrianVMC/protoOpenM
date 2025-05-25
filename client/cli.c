#include <stdio.h>
#include <string.h>
#include "../include/cli.h"
#include "../include/data.h"

void show_main_menu() {
    printf("=== OpenMS ===\n");
    printf("1. Registrarse\n");
    printf("2. Iniciar sesión\n");
    printf("3. Ver canciones\n");
    printf("0. Salir\n");
    printf("Seleccione una opción: ");
}

void handle_main_menu() {
    int option;
    char user[50], pass[50];

    do {
        show_main_menu();
        scanf("%d", &option);
        getchar();

        switch (option) {
            case 1:
                printf("Usuario: "); fgets(user, sizeof(user), stdin);
                printf("Contraseña: "); fgets(pass, sizeof(pass), stdin);
                user[strcspn(user, "\n")] = 0;
                pass[strcspn(pass, "\n")] = 0;
                register_user(user, pass);
                break;

            case 2:
                printf("Usuario: "); fgets(user, sizeof(user), stdin);
                printf("Contraseña: "); fgets(pass, sizeof(pass), stdin);
                user[strcspn(user, "\n")] = 0;
                pass[strcspn(pass, "\n")] = 0;
                if (login_user(user, pass))
                    printf("Inicio de sesión exitoso\n");
                else
                    printf("Error al iniciar sesión\n");
                break;

            case 3:
                list_songs();
                break;
        }
    } while (option != 0);
}

int main() {
    handle_main_menu();
    return 0;
}