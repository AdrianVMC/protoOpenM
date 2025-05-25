#ifndef DATA_H
#define DATA_H

extern char CURRENT_USER[50];

typedef struct {
    char username[50];
    char password[50];
} User;

typedef struct {
    char title[100];
    char artist[100];
} Song;

void hide_password(char *buffer, size_t max_length);
int register_user(const char *username, const char *password);
int login_user(const char *username, const char *password);
void list_songs();
int load_users();
int load_songs();

#endif
