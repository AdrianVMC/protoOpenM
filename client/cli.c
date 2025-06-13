<<<<<<< Updated upstream
=======
#include "config.h"
>>>>>>> Stashed changes
#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "../include/shared.h"
#include "../include/shared_utils.h"
#include "../include/client_registry.h"

<<<<<<< Updated upstream
#define LOGIN_INPUT_MAX 64
#define MAX_SONGS 50
#define NUM_OPTIONS 4

const char *menu_options[NUM_OPTIONS] = {
    "View songs",
    "Search song",
    "Reproduce song",
    "Logout"
};

void clean_screen() {
    printf("\033[H\033[J");
}

int get_mp3_duration(const char *filename) {
    char command[256];
    snprintf(command, sizeof(command),
             "ffprobe -v error -show_entries format=duration -of csv=p=0 \"%s\"", filename);

    FILE *fp = popen(command, "r");
    if (!fp) return 30;

    char buffer[64];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        pclose(fp);
        return 30;
    }

    pclose(fp);
    return (int)(atof(buffer) + 0.5);
}

void show_progress_bar(int duration, pid_t player_pid) {
    int paused = 0;
    int elapsed = 0;
    time_t start = time(NULL);
    time_t pause_start = 0;
    int total_paused_time = 0;

    nodelay(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    while (elapsed < duration) {
        int ch = getch();

        switch (ch) {
            case 'p':
            case 'P':
                if (!paused) {
                    kill(player_pid, SIGSTOP);
                    paused = 1;
                    pause_start = time(NULL);
                }
            break;
            case 'r':
            case 'R':
                if (paused) {
                    kill(player_pid, SIGCONT);
                    paused = 0;
                    total_paused_time += (int)(time(NULL) - pause_start);
                }
            break;
            case 'q':
            case 'Q':
                kill(player_pid, SIGTERM);
            waitpid(player_pid, NULL, 0);
            nodelay(stdscr, FALSE);
            curs_set(1);
            return;
        }

        if (!paused) {
            time_t now = time(NULL);
            elapsed = (int)(now - start - total_paused_time);
        }

        clear();
        mvprintw(2, 2, "Playing song...");
        mvprintw(3, 2, "Status: %s", paused ? "Paused" : "Playing");
        mvprintw(5, 2, "Controls: [P]ause | [R]esume | [Q]uit");

        mvprintw(7, 2, "[");
        int bar_width = 50;
        int pos = (elapsed * bar_width) / duration;
        if (pos > bar_width) pos = bar_width;

        for (int i = 0; i < bar_width; i++) {
            printw(i < pos ? "=" : " ");
        }
        printw("] %d/%d sec", elapsed, duration);
        refresh();
    }

    nodelay(stdscr, FALSE);
    curs_set(1);
    waitpid(player_pid, NULL, 0);
}




void prompt_song_name(char *buffer, size_t size) {
    clean_screen();
    printf("Enter song's exact name: ");
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
}

void download_and_play_song(SharedData *data, sem_t *client_sem, sem_t *server_sem, const char *song_name) {
    clean_screen();
    snprintf(data->message, MAX_MSG, "GET|%s", song_name);
    sem_post(client_sem);

    char temp_filename[128];
    snprintf(temp_filename, sizeof(temp_filename), "/tmp/%s_temp.mp3", song_name);
    FILE *fp = fopen(temp_filename, "wb");

    while (1) {
        sem_wait(server_sem);
        fwrite(data->audio_chunk, 1, data->chunk_size, fp);
        sem_post(client_sem);
        if (data->is_last_chunk) break;
    }

    fclose(fp);

    initscr();
    noecho();
    cbreak();

    pid_t pid = fork();
    if (pid == 0) {
        execlp("mpg123", "mpg123", "--quiet", temp_filename, NULL);
        perror("Error al ejecutar mpg123");
        exit(1);
    } else {
        int duration = get_mp3_duration(temp_filename);
        show_progress_bar(duration, pid);
    }

    endwin();
    remove(temp_filename);
    clean_screen();
}

void play_song_by_name(SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    clean_screen();
    char song_name[64];
    prompt_song_name(song_name, sizeof(song_name));

    if (strlen(song_name) == 0) {
        printf("No name given.\nPress Enter to go back to menu...");
        getchar();
        return;
    }

    download_and_play_song(data, client_sem, server_sem, song_name);
    printf("\nReproduction Finalized. Press Enter to get back to menu...");
    getchar();
}


void login_interface(char *username, char *password) {
    clean_screen();
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    keypad(stdscr, TRUE);

    int y = 5, x = 10;

    int exit_flag = 0;
    do {
        clear();
        mvprintw(y, x, "Login System");
        mvprintw(y + 2, x, "Username (cannot be empty): ");
        mvprintw(y + 8, x, "Press ESC to exit");
        echo();
        move(y + 2, x + 29);

        int ch, i = 0;
        while ((ch = getch()) != '\n' && i < LOGIN_INPUT_MAX - 1) {
            if (ch == 27) {
                exit_flag = 1;
                break;
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (i > 0) {
                    i--;
                    mvaddch(y + 2, x + 29 + i, ' ');
                    move(y + 2, x + 29 + i);
                }
            } else if (ch >= 32 && ch <= 126) {
                username[i++] = ch;
                mvaddch(y + 2, x + 29 + i - 1, ch);
            }
        }
        username[i] = '\0';
        noecho();
        if (exit_flag) break;
    } while (strlen(username) == 0);

    if (!exit_flag) {
        int valid_password = 0;
        while (!valid_password) {
            memset(password, 0, LOGIN_INPUT_MAX);
            mvprintw(y + 4, x, "                                 ");
            mvprintw(y + 4, x, "Password (cannot be empty): ");
            move(y + 4, x + 29);

            int i = 0, ch;
            while ((ch = getch()) != '\n' && i < LOGIN_INPUT_MAX - 1) {
                if (ch == 27) {
                    exit_flag = 1;
                    break;
                } else if (ch == KEY_BACKSPACE || ch == 127) {
                    if (i > 0) {
                        i--;
                        mvaddch(y + 4, x + 29 + i, ' ');
                        move(y + 4, x + 29 + i);
                    }
                } else if (ch >= 32 && ch <= 126) {
                    password[i++] = ch;
                    mvaddch(y + 4, x + 29 + i - 1, '*');
                }
            }
            password[i] = '\0';
            if (exit_flag) break;

            move(y + 6, x);
            clrtoeol();
            if (strlen(password) == 0) {
                mvprintw(y + 6, x, "Password cannot be empty. Try again.");
            } else {
                valid_password = 1;
            }
        }
    }

    if (!exit_flag) {
        mvprintw(y + 6, x, "Authenticating...");
        refresh();
        sleep(1);
    }

    clear();
    endwin();

    if (exit_flag) {
        username[0] = '\0';
        password[0] = '\0';
    }
}






void view_songs(SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    clean_screen();
    snprintf(data->message, MAX_MSG, "SONGS");
    sem_post(client_sem);
    sem_wait(server_sem);

    clean_screen();
    if (strncmp(data->message, "SONGS|", 6) == 0) {
        char *list = data->message + 6;
        char *song = strtok(list, ";");
        int index = 1;
        printf("\nAvailable songs:\n");
        while (song) {
            printf("  %2d. %s\n", index++, song);
            song = strtok(NULL, ";");
        }
    } else {
        printf("%s\n", data->message);
    }

    printf("\nPress Enter to return to the menu ...");
    getchar();
}

void search_song(SharedData *data, sem_t *client_sem, sem_t *server_sem) {
    clean_screen();
    char search_term[64];
    printf("\nEnter the name or part of the song title: ");
    fgets(search_term, sizeof(search_term), stdin);
    search_term[strcspn(search_term, "\n")] = '\0';

    snprintf(data->message, MAX_MSG, "SEARCH|%s", search_term);
    sem_post(client_sem);
    sem_wait(server_sem);

    if (strncmp(data->message, "FOUND|", 6) == 0) {
        char *list = data->message + 6;
        char *song = strtok(list, ";");
        int index = 1;
        printf("\nSearch results:\n");
        while (song) {
            printf("  %2d. %s\n", index++, song);
            song = strtok(NULL, ";");
        }
    } else {
        printf("%s\n", data->message);
    }

    printf("\nPress ENTER to return to the menu...");
    getchar();
}


int show_main_menu_ncurses() {
    clean_screen();
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    int highlight = 0;
    int input;
    int selected = -1;

    while (1) {
        clear();
        mvprintw(1, 2, "Main Menu");

        for (int i = 0; i < NUM_OPTIONS; i++) {
            if (i == highlight) {
                attron(A_REVERSE);
                mvprintw(3 + i, 4, "> %s", menu_options[i]);
                attroff(A_REVERSE);
            } else {
                mvprintw(3 + i, 6, "  %s", menu_options[i]);
            }
        }

        refresh();
        input = getch();
        switch (input) {
            case KEY_UP:
                highlight = (highlight == 0) ? NUM_OPTIONS - 1 : highlight - 1;
            break;
            case KEY_DOWN:
                highlight = (highlight + 1) % NUM_OPTIONS;
            break;
            case '\n':
                selected = highlight;
            endwin();
            return selected;
            case 27:
                endwin();
            return -1;
        }
    }
}



int main() {
    char username[LOGIN_INPUT_MAX];
    char password[LOGIN_INPUT_MAX];
    pid_t pid = getpid();

    char shm_name[64], sem_client[64], sem_server[64];
    generate_names(shm_name, sem_client, sem_server, pid);

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedData));
    SharedData *data = mmap(0, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    sem_t *client_sem = sem_open(sem_client, O_CREAT, 0666, 0);
    sem_t *server_sem = sem_open(sem_server, O_CREAT, 0666, 0);

    register_client(pid);

    login_interface(username, password);
    snprintf(data->message, MAX_MSG, "LOGIN|%s|%s", username, password);
    sem_post(client_sem);
    sem_wait(server_sem);

    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("Login cancelled.\n");
    }

    if (strcmp(data->message, "OK") == 0) {
        printf("\n️Welcome, %s\n", username);

        int exit_program = 0;
        while (!exit_program) {
            int option = show_main_menu_ncurses();

            switch (option) {
                case 0:
                    view_songs(data, client_sem, server_sem);
                    break;
                case 1:
                    search_song(data, client_sem, server_sem);
                    break;
                case 2:
                    play_song_by_name(data, client_sem, server_sem);
                    break;
                case 3:
                    snprintf(data->message, MAX_MSG, "LOGOUT");
                    sem_post(client_sem);
                    exit_program = 1;
                    break;
                default:
                    printf("Invalid option\n");
            }
=======
#define NUM_MAIN 5
static const char *main_opt[NUM_MAIN] = {
    "View songs", "Search song", "Reproduce song", "My Playlist", "Logout"
};

#define MAX_LINES 256

void clear_screen() {
    system("clear");//Falta cambiarlo por printf asci
}

static void ncurses_setup() {
    clear_screen();
    initscr(); start_color(); cbreak(); noecho(); curs_set(0); keypad(stdscr, TRUE);
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);
    init_pair(3, COLOR_YELLOW, COLOR_BLUE);
    bkgd(COLOR_PAIR(1)); refresh();
}

static void show_list_window(const char *title, const char **lines, int count) {
    clear_screen();
    ncurses_setup();
    int width = 50, height = count + 6;
    if (height > LINES - 2) height = LINES - 2;
    int startx = (COLS - width) / 2, starty = (LINES - height) / 2;
    WINDOW *win = newwin(height, width, starty, startx);
    wbkgd(win, COLOR_PAIR(1)); box(win, 0, 0);

    wattron(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, 1, (width - strlen(title)) / 2, "%s", title);
    wattroff(win, COLOR_PAIR(3) | A_BOLD);

    for (int i = 0; i < count && i < height - 4; i++) {
        mvwprintw(win, 3 + i, 2, "%2d. %s", i + 1, lines[i]);
    }

    mvwprintw(win, height - 2, (width - 24) / 2, "[ENTER] para continuar");
    wrefresh(win);
    getch(); delwin(win); endwin();
}

static void input_window(const char *prompt, char *output, int max_len) {
    clear_screen();
    ncurses_setup();
    int width = 50, height = 7;
    WINDOW *win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    wbkgd(win, COLOR_PAIR(1)); box(win, 0, 0);

    wattron(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, 1, 2, "%s", prompt);
    wattroff(win, COLOR_PAIR(3) | A_BOLD);

    echo();
    mvwprintw(win, 3, 2, "> ");
    wmove(win, 3, 4);
    wgetnstr(win, output, max_len - 1);
    noecho();

    delwin(win); endwin();
}


static void form_ui(const char *title, char *u, char *p) {
    clear_screen();
    ncurses_setup(); curs_set(1);
    int width = 40, height = 10;
    WINDOW *win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    wbkgd(win, COLOR_PAIR(1)); box(win, 0, 0);

    wattron(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, 1, (width - strlen(title)) / 2, "%s", title);
    wattroff(win, COLOR_PAIR(3) | A_BOLD);

    mvwprintw(win, 3, 2, "Username: ");
    echo(); wmove(win, 3, 12); wgetnstr(win, u, LOGIN_INPUT_MAX - 1); noecho();

    mvwprintw(win, 5, 2, "Password: ");
    wmove(win, 5, 12);
    int i = 0, ch;
    while ((ch = wgetch(win)) != '\n') {
        if ((ch == KEY_BACKSPACE || ch == 127) && i > 0) {
            --i; mvwaddch(win, 5, 12 + i, ' '); wmove(win, 5, 12 + i);
        } else if (ch >= 32 && ch <= 126 && i < LOGIN_INPUT_MAX - 1) {
            p[i++] = ch; mvwaddch(win, 5, 12 + i - 1, '*');
        }
    }
    p[i] = '\0';
    delwin(win); endwin();
}

static int start_menu(void) {
    clear_screen();
    const char *opts[] = { "Login", "Register", "Exit" };
    int hl = 0, ch, width = 30, height = 9;
    ncurses_setup();
    WINDOW *win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    wbkgd(win, COLOR_PAIR(1)); box(win, 0, 0);

    while (1) {
        werase(win); box(win, 0, 0);
        wattron(win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(win, 1, (width - 12) / 2, "protoOpenM");
        wattroff(win, COLOR_PAIR(3) | A_BOLD);

        for (int i = 0; i < 3; i++) {
            if (i == hl) wattron(win, COLOR_PAIR(2));
            mvwprintw(win, 3 + i, 3, " %s", opts[i]);
            if (i == hl) wattroff(win, COLOR_PAIR(2));
        }

        wrefresh(win);
        ch = getch();
        if (ch == KEY_UP) hl = (hl == 0) ? 2 : hl - 1;
        else if (ch == KEY_DOWN) hl = (hl + 1) % 3;
        else if (ch == '\n') { delwin(win); endwin(); return hl; }
    }
}

static int menu_ncurses(void) {
    clear_screen();
    int hl = 0, ch, width = 40, height = NUM_MAIN + 5;
    ncurses_setup();
    WINDOW *win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    wbkgd(win, COLOR_PAIR(1)); box(win, 0, 0);

    while (1) {
        werase(win); box(win, 0, 0);
        wattron(win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(win, 1, (width - 13) / 2, "Principal Menu");
        wattroff(win, COLOR_PAIR(3) | A_BOLD);

        for (int i = 0; i < NUM_MAIN; i++) {
            if (i == hl) wattron(win, COLOR_PAIR(2));
            mvwprintw(win, 3 + i, 2, " > %s", main_opt[i]);
            if (i == hl) wattroff(win, COLOR_PAIR(2));
        }

        wrefresh(win);
        ch = getch();
        if (ch == KEY_UP) hl = (hl == 0) ? NUM_MAIN - 1 : hl - 1;
        else if (ch == KEY_DOWN) hl = (hl + 1) % NUM_MAIN;
        else if (ch == '\n') { delwin(win); endwin(); return hl; }
        else if (ch == 27)    { delwin(win); endwin(); return -1; }
    }
}

static void login_ui(char *u, char *p, SharedData *d, sem_t *c, sem_t *s) {
    clear_screen();
    form_ui("Login", u, p);
    if (u[0] && p[0]) {
        snprintf(d->message, MAX_MSG, "LOGIN|%s|%s", u, p);
        sem_post(c); sem_wait(s);
    } else {
        u[0] = p[0] = '\0';
    }
}

static void reg_remote(const char *u, const char *p, SharedData *d, sem_t *c, sem_t *s) {
    clear_screen();
    char hash[HASH_STRING_LENGTH]; hash_sha256(p, hash);
    snprintf(d->message, MAX_MSG, "REGISTER|%s|%s", u, hash);
    sem_post(c); sem_wait(s);
    printf("%s\n", strcmp(d->message, "OK") == 0 ? "Created!" : "Failed.");
    getchar();
}

static void reg_ui(char *u, char *p, SharedData *d, sem_t *c, sem_t *s) {
    clear_screen();
    form_ui("Registro", u, p);
    if (u[0] && p[0]) reg_remote(u, p, d, c, s);
}

static void view(SharedData *d, sem_t *c, sem_t *s) {
    clear_screen();
    snprintf(d->message, MAX_MSG, "SONGS"); sem_post(c); sem_wait(s);
    if (!strncmp(d->message, "SONGS|", 6)) {
        char *lines[MAX_LINES]; int count = 0;
        char list[MAX_MSG]; strncpy(list, d->message + 6, sizeof list);
        char *t = strtok(list, ";");
        while (t && count < MAX_LINES) {
            lines[count++] = t;
            t = strtok(NULL, ";");
        }
        show_list_window("Available songs", (const char **)lines, count);
    } else {
        const char *msg[] = { d->message };
        show_list_window("Error", msg, 1);
    }
}

static void search(SharedData *d, sem_t *c, sem_t *s) {
    clear_screen();
    char term[64];
    input_window("Enter the search term:", term, sizeof(term));

    snprintf(d->message, MAX_MSG, "SEARCH|%s", term);
    sem_post(c); sem_wait(s);

    if (!strncmp(d->message, "FOUND|", 6)) {
        char *lines[MAX_LINES]; int count = 0;
        char list[MAX_MSG]; strncpy(list, d->message + 6, sizeof list);
        char *t = strtok(list, ";");
        while (t && count < MAX_LINES) {
            lines[count++] = t;
            t = strtok(NULL, ";");
        }
        show_list_window("Found results", (const char **)lines, count);
    } else {
        const char *msg[] = { d->message };
        show_list_window("Result", msg, 1);
    }
}


static void pl_menu(SharedData *d, sem_t *c, sem_t *s) {
    clear_screen();
    const char *opts[] = {
        "Show playlist", "Add song", "Delete song", "Play playlist", "Return"
    };
    int hl = 0, ch, width = 40, height = 11;
    ncurses_setup();
    WINDOW *win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
    wbkgd(win, COLOR_PAIR(1));

    while (1) {
        werase(win); box(win, 0, 0);
        wattron(win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(win, 1, (width - 17) / 2, "Menu Playlist");
        wattroff(win, COLOR_PAIR(3) | A_BOLD);

        for (int i = 0; i < 5; i++) {
            if (i == hl) wattron(win, COLOR_PAIR(2));
            mvwprintw(win, 3 + i, 2, " > %s", opts[i]);
            if (i == hl) wattroff(win, COLOR_PAIR(2));
        }

        wrefresh(win);
        ch = getch();
        if (ch == KEY_UP) hl = (hl == 0) ? 4 : hl - 1;
        else if (ch == KEY_DOWN) hl = (hl + 1) % 5;
        else if (ch == '\n') {
            delwin(win); endwin();
            if (hl == 0) {
                snprintf(d->message, MAX_MSG, "PLIST|SHOW"); sem_post(c); sem_wait(s);
                if (!strncmp(d->message, "PLIST|", 6)) {
                    char *lines[MAX_LINES]; int count = 0;
                    char list[MAX_MSG]; strncpy(list, d->message + 6, sizeof list);
                    char *t = strtok(list, ";");
                    while (t && count < MAX_LINES) {
                        lines[count++] = t;
                        t = strtok(NULL, ";");
                    }
                    show_list_window("My Playlist", (const char **)lines, count);
                } else {
                    const char *msg[] = { "Playlist empty" };
                    show_list_window("My Playlist", msg, 1);
                }
            } else if (hl == 1 || hl == 2) {
                char t[128];
                printf("Title: "); fgets(t, sizeof t, stdin);
                t[strcspn(t, "\n")] = '\0';
                if (t[0]) {
                    snprintf(d->message, MAX_MSG, hl == 1 ? "PLIST|ADD|%s" : "PLIST|DEL|%s", t);
                    sem_post(c); sem_wait(s);
                    const char *msg[] = { hl == 1 ? "Added." : "Deleted." };
                    show_list_window("Result", msg, 1);
                }
            } else if (hl == 3) {
                const char *msg[] = { "[Simulating play]" };
                show_list_window("Playing", msg, 1);
            } else if (hl == 4) return;

            ncurses_setup();
            win = newwin(height, width, (LINES - height) / 2, (COLS - width) / 2);
            wbkgd(win, COLOR_PAIR(1));
        }
    }
}

static int mp3_len(const char *f) {
    char cmd[256];
    snprintf(cmd, sizeof cmd,
        "ffprobe -v error -show_entries format=duration -of csv=p=0 \"%s\"", f);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 30;
    char buf[64];
    if (!fgets(buf, sizeof buf, fp)) {
        pclose(fp);
        return 30;
    }
    pclose(fp);
    return (int)(atof(buf) + 0.5);
}

static void bar_ncurses(int len, pid_t pid) {
    int pause = 0, elap = 0, tot = 0;
    time_t st = time(NULL), ps = 0;

    initscr();
    resize_term(0, 0);
    start_color();
    noecho();
    cbreak();
    curs_set(0);

    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_CYAN);
    init_pair(3, COLOR_YELLOW, COLOR_BLUE);

    bkgd(COLOR_PAIR(1));
    refresh();

    int height = 12, width = 70;
    if (LINES < height || COLS < width) {
        endwin();
        fprintf(stderr, "Error size %dx%d.\n", width, height);
        return;
    }

    int starty = (LINES - height) / 2;
    int startx = (COLS - width) / 2;
    WINDOW *win = newwin(height, width, starty, startx);
    wbkgd(win, COLOR_PAIR(1));
    keypad(win, TRUE);
    nodelay(win, TRUE);
    halfdelay(1);

    while (elap < len) {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, 2, "Reproduce (%s)", pause ? "PAUSED" : "PLAYING");
        mvwprintw(win, 3, 2, "[P] Pause   [R] Resume   [Q] Quit");
        int bar_width = 50;
        int progress = (elap * bar_width) / len;
        mvwprintw(win, 5, 2, "[");
        wattron(win, COLOR_PAIR(2));
        for (int i = 0; i < progress; i++) {
            waddch(win, ACS_CKBOARD);
        }
        wattroff(win, COLOR_PAIR(2));
        for (int i = progress; i < bar_width; i++) {
            waddch(win, ' ');
        }
        wprintw(win, "]");
        mvwprintw(win, 6, 2, "%d/%d seg", elap, len);
        mvwprintw(win, height - 2, 2, "Press Q to Quit");
        wrefresh(win);
        int ch = ERR;
        while ((ch = wgetch(win)) != ERR) {
            if ((ch == 'p' || ch == 'P') && !pause) {
                kill(pid, SIGSTOP);
                pause = 1;
                ps = time(NULL);
                break;
            } else if ((ch == 'r' || ch == 'R') && pause) {
                kill(pid, SIGCONT);
                pause = 0;
                tot += (int)(time(NULL) - ps);
                break;
            } else if (ch == 'q' || ch == 'Q') {
                kill(pid, SIGTERM);
                goto cleanup;
            }
        }
        if (!pause) {
            elap = (int)(time(NULL) - st - tot);
            if (elap < 0) elap = 0;
            if (elap > len) elap = len;
        }
        napms(50);
    }
cleanup:
    curs_set(1);
    delwin(win);
    endwin();
    waitpid(pid, NULL, 0);
}


static void play_single(SharedData *d, sem_t *c, sem_t *s) {
    char song[64];
    input_window("Title of the song:", song, sizeof(song));
    if (!song[0]) return;

    snprintf(d->message, MAX_MSG, "GET|%s", song);
    sem_post(c);

    char tmp[128];
    snprintf(tmp, sizeof tmp, "/tmp/%s_tmp.mp3", song);
    FILE *fp = fopen(tmp, "wb");
    if (!fp) {
        const char *msg[] = { "Error creando archivo temporal." };
        show_list_window("Error", msg, 1);
        return;
    }

    while (1) {
        sem_wait(s);
        fwrite(d->audio_chunk, 1, d->chunk_size, fp);
        sem_post(c);
        if (d->is_last_chunk) break;
    }
    fclose(fp);

    pid_t pid = fork();
    if (pid == 0) {
        execlp("mpg123", "mpg123", "--quiet", tmp, NULL);
        perror("mpg123");
        exit(1);
    } else {
        int duration = mp3_len(tmp);
        if (duration <= 0) duration = 30; // Fallback duration
        bar_ncurses(duration, pid);
        remove(tmp);
    }
}


int main(void) {
    clear_screen();
    char user[LOGIN_INPUT_MAX] = {0}, pass[LOGIN_INPUT_MAX] = {0};
    pid_t pid = getpid();
    char shm[64], sc[64], ss[64];
    generate_names(shm, sc, ss, pid);
    int fd = shm_open(shm, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedData));
    SharedData *d = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    sem_t *csem = sem_open(sc, O_CREAT, 0666, 0), *ssem = sem_open(ss, O_CREAT, 0666, 0);
    register_client(pid);

    while (1) {
        int ch = start_menu();
        if (ch == 2) goto end;
        if (ch == 1) { reg_ui(user, pass, d, csem, ssem); continue; }
        if (ch == 0) break;
    }

    login_ui(user, pass, d, csem, ssem);
    if (strcmp(d->message, "OK") != 0) { puts("Auth failed."); goto end; }

    int quit = 0;
    while (!quit) {
        int op = menu_ncurses();
        switch (op) {
            case 0: view(d, csem, ssem); break;
            case 1: search(d, csem, ssem); break;
            case 2: play_single(d, csem, ssem); break;
            case 3: pl_menu(d, csem, ssem); break;
            case 4: snprintf(d->message, MAX_MSG, "LOGOUT"); sem_post(csem); quit = 1; break;
            default: quit = 1; break;
>>>>>>> Stashed changes
        }
    } else {
        printf("\nAuthentication failed.\n");
    }

<<<<<<< Updated upstream
    munmap(data, sizeof(SharedData));
    close(shm_fd);
    sem_close(client_sem);
    sem_close(server_sem);
    shm_unlink(shm_name);
    sem_unlink(sem_client);
    sem_unlink(sem_server);

=======
end:
    munmap(d, sizeof(SharedData)); close(fd);
    sem_close(csem); sem_close(ssem);
    shm_unlink(shm); sem_unlink(sc); sem_unlink(ss);
>>>>>>> Stashed changes
    return 0;
}
