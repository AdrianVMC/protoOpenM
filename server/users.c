#define _POSIX_C_SOURCE 200809L
#include "users.h"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

static const char *DB = "data/users.dat";
static sem_t sem;                       /* mutex proceso-único */

static void lock  (void){ sem_wait (&sem); }
static void unlock(void){ sem_post (&sem); }

static void sha256(const uint8_t *in, size_t n, uint8_t out[HASH_LEN]){
    SHA256(in, n, out);
}

static void salted_hash(const uint8_t salt[SALT_LEN],
                        const char *pw,
                        uint8_t out[HASH_LEN])
{
    uint8_t buf[SALT_LEN + USER_MAX];
    memcpy(buf, salt, SALT_LEN);
    strncpy((char*)buf + SALT_LEN, pw, USER_MAX);
    sha256(buf, SALT_LEN + strlen(pw), out);
}

/*--------------------------------------------------------------------*/

int users_init(void){
    sem_init(&sem, 0, 1);
    FILE *f = fopen(DB, "ab");          /* crea si no existe */
    if(!f) return -1;
    fclose(f);
    return 0;
}

/* devuelve 1 si existe (user copiado en *slot, posición en *pos) */
static int find_user(const char *u, User *slot, long *pos){
    FILE *f = fopen(DB, "rb");
    if(!f) return -1;
    long off = 0;
    User tmp;
    while(fread(&tmp, sizeof(User), 1, f) == 1){
        if(strcmp(tmp.username, u) == 0){
            if(slot) *slot = tmp;
            if(pos)  *pos  = off;
            fclose(f);
            return 1;
        }
        off += sizeof(User);
    }
    fclose(f);
    return 0;
}

int users_register(const char *u, const char *pw){
    if(strlen(u) >= USER_MAX) return -2;
    lock();
        if(find_user(u, NULL, NULL)){ unlock(); return -3; } /* ya existe */

        User n = {0};
        strcpy(n.username, u);
        RAND_bytes(n.salt, SALT_LEN);
        salted_hash(n.salt, pw, n.hash);
        n.session = 0;

        FILE *f = fopen(DB, "ab");
        if(!f){ unlock(); return -4; }
        fwrite(&n, sizeof(User), 1, f);
        fclose(f);
    unlock();
    return 0;
}

int users_login(const char *u, const char *pw){
    User cur; long pos;
    lock();
        if(!find_user(u, &cur, &pos)){ unlock(); return -1; } /* no existe */
        if(cur.session){ unlock(); return -2; }               /* duplicada */

        uint8_t h[HASH_LEN];
        salted_hash(cur.salt, pw, h);
        if(memcmp(h, cur.hash, HASH_LEN)!=0){ unlock(); return -3; }

        cur.session = 1;                                      /* marcar */
        FILE *f = fopen(DB, "r+b"); fseek(f, pos, SEEK_SET);
        fwrite(&cur, sizeof(User), 1, f); fclose(f);
    unlock();
    return 0;
}

void users_logout(const char *u){
    User cur; long pos;
    lock();
        if(!find_user(u, &cur, &pos)){ unlock(); return; }
        cur.session = 0;
        FILE *f = fopen(DB, "r+b"); fseek(f, pos, SEEK_SET);
        fwrite(&cur, sizeof(User), 1, f); fclose(f);
    unlock();
}
