#ifndef DATA_H
#define DATA_H

typedef struct {
    char username[50];
    char password[50];
} User;

typedef struct {
    char title[100];
    char artist[100];
} Song;

int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password);
void list_songs();
int load_users();
int load_songs();

#endif
