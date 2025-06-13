
#ifndef HASH_UTILS_H
#define HASH_UTILS_H

<<<<<<< Updated upstream
#define HASH_STRING_LENGTH 65 // 64 hex chars + null terminator
=======
#include "config.h"
>>>>>>> Stashed changes

void hash_sha256(const char *input, char output[HASH_STRING_LENGTH]);
int compare_hashes(const char *input, const char *stored_hash);

#endif
