# ───────────────────────────── protoOpenM Makefile ─────────────────────────────

CC      = gcc
CFLAGS  = -Wall -pthread -Iinclude
LDFLAGS = -lssl -lcrypto
NCURSES = -lncursesw       # solo el cliente lo necesita

BUILD_DIR = build

# ──────────────── Fuentes comunes ────────────────
UTILS_SRC    = utils/shared_utils.c utils/client_registry.c utils/hash_utils.c
AUTH_SRC   = utils/authenticate.c            # ← nuevo

# ──────────────── Objetivos ─────────────────────
CLIENT_SRC   = client/cli.c
SERVER_SRC   = server/server.c

CLIENT_BIN   = $(BUILD_DIR)/client
SERVER_BIN   = $(BUILD_DIR)/server

.PHONY: all clean run-client run-server

all: $(CLIENT_BIN) $(SERVER_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---------- Cliente ----------
$(CLIENT_BIN): $(CLIENT_SRC) $(UTILS_SRC) $(AUTH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(NCURSES) $(LDFLAGS)

# ---------- Servidor ----------
$(SERVER_BIN): $(SERVER_SRC) $(UTILS_SRC) $(AUTH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

clean:
	rm -rf $(BUILD_DIR)
