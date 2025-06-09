#include "../include/hash_utils.h"
#include <openssl/sha.h>
#include <string.h>
#include <stdio.h>

void hash_sha256(const char *input, char output[HASH_STRING_LENGTH]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input, strlen(input), hash);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = '\0';
}

int compare_hashes(const char *input, const char *stored_hash) {
    char hash[HASH_STRING_LENGTH];
    hash_sha256(input, hash);
    return strcmp(hash, stored_hash) == 0;
}
