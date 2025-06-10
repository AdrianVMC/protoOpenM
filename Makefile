CC      = gcc
OPENSSL_DIR = /opt/homebrew/opt/openssl@3
CFLAGS  = -Wall -pthread -Iinclude -I$(OPENSSL_DIR)/include
LDFLAGS = -L$(OPENSSL_DIR)/lib -lssl -lcrypto
NCURSES = -lncurses

BUILD_DIR = build

# ──────────────── Fuentes comunes ────────────────
UTILS_SRC    = utils/shared_utils.c utils/client_registry.c utils/hash_utils.c
AUTH_SRC     = utils/authenticate.c

# ──────────────── Objetivos ─────────────────────
CLIENT_SRC = client/cli.c
SERVER_SRC = server/server.c

CLIENT_BIN = $(BUILD_DIR)/client
SERVER_BIN = $(BUILD_DIR)/server

.PHONY: all clean run-client run-server

all: $(CLIENT_BIN) $(SERVER_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CLIENT_BIN): $(CLIENT_SRC) $(UTILS_SRC) $(AUTH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(NCURSES) $(LDFLAGS)

$(SERVER_BIN): $(SERVER_SRC) $(UTILS_SRC) $(AUTH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

clean:
	rm -rf $(BUILD_DIR)
