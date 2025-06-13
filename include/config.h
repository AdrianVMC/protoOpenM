#ifndef CONFIG_H
#define CONFIG_H
#define MAX_MSG  1024
#define AUDIO_CHUNK_SIZE  4096
#define LOGIN_INPUT_MAX  64
#define HASH_STRING_LENGTH  65
#define MAX_CLIENTS  10
#define SHM_NAME_LEN  64
#define SEM_NAME_LEN  64
#define SHM_PREFIX  "/client_shm_"
#define SEM_CLIENT_PREFIX "/client_sem_"
#define SEM_SERVER_PREFIX "/server_sem_"
#define REGISTRY_SHM_NAME  "/client_registry_shm"
#define REGISTRY_SEM_NAME "/client_registry_sem"
#define DATADIR  "data"
#endif
