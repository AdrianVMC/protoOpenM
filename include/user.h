#ifndef USERS_H
#define USERS_H

#include <stdint.h>

#define USER_MAX   50
#define SALT_LEN   16
#define HASH_LEN   32

typedef struct {
    char     username[USER_MAX];
    uint8_t  salt[SALT_LEN];
    uint8_t  hash[HASH_LEN];
    int8_t   session;
} User;


int  users_init(void);
int  users_register(const char *u, const char *pw);
int  users_login   (const char *u, const char *pw);
void users_logout  (const char *u);

#endif
