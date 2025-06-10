#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include "config.h"     /* HASH_STRING_LENGTH vive aquí */

void hash_sha256(const char *input, char output[HASH_STRING_LENGTH]);
int  compare_hashes(const char *input, const char *stored_hash);

#endif
