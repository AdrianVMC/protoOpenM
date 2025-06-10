/*====================================================================
*  config.h  –  Parámetros globales para todo el proyecto
 *====================================================================*/
#ifndef CONFIG_H
#define CONFIG_H

/* ─────────── Buffers y tamaños generales ─────────── */
#define MAX_MSG               1024   /* bytes en el canal de comandos */
#define AUDIO_CHUNK_SIZE      4096   /* bytes por fragmento de audio  */
#define LOGIN_INPUT_MAX         64   /* máx. de caracteres usuario/psw*/
#define HASH_STRING_LENGTH      65   /* 64 hex + ‘\0’                 */

/* ─────────── Registro de clientes ─────────── */
#define MAX_CLIENTS             10   /* slots en el registro compartido */

/* ─────────── Nombres de SHM y semáforos ────── */
#define SHM_NAME_LEN            64
#define SEM_NAME_LEN            64

#define SHM_PREFIX             "/client_shm_"
#define SEM_CLIENT_PREFIX      "/client_sem_"
#define SEM_SERVER_PREFIX      "/server_sem_"      /* ← prefijo definitivo */

#define REGISTRY_SHM_NAME      "/client_registry_shm"
#define REGISTRY_SEM_NAME      "/client_registry_sem"

/* ─────────── Ruta base a los archivos de datos ───────────
 *  Usa ruta absoluta si el binario se ejecutará desde carpetas distintas.
 */
#define DATADIR                "data"

#endif /* CONFIG_H */
