#ifndef PLAYLIST_H
#define PLAYLIST_H
void playlist_handle(SharedData *d,
                     const char *current_user,
                     sem_t *client_sem,
                     sem_t *server_sem);
#endif
