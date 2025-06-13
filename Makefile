<<<<<<< Updated upstream
# Makefile para protoOpenM con registro automático y ncurses

CC = gcc
CFLAGS = -Wall -pthread -Iinclude

NCURSES_FLAGS = -lncursesw
LDFLAGS += -lssl -lcrypto

BUILD_DIR = build

=======
CC = gcc
OPENSSL_DIR = /opt/homebrew/opt/openssl@3
CFLAGS  = -Wall -pthread -Iinclude -I$(OPENSSL_DIR)/include
LDFLAGS = -L$(OPENSSL_DIR)/lib -lssl -lcrypto
NCURSES = -lncurses

BUILD_DIR = build

UTILS_SRC = utils/shared_utils.c utils/client_registry.c utils/hash_utils.c
AUTH_SRC  = utils/authenticate.c

>>>>>>> Stashed changes
CLIENT_SRC = client/cli.c
SERVER_SRC = server/server.c
UTILS_SRC = utils/shared_utils.c
REGISTRY_SRC = utils/client_registry.c
HASH_SRC = utils/hash_utils.c
USER_SRC = utils/user_registry.c

CLIENT_BIN = $(BUILD_DIR)/client
SERVER_BIN = $(BUILD_DIR)/server

.PHONY: all clean run-client run-server

all: $(CLIENT_BIN) $(SERVER_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CLIENT_BIN): $(CLIENT_SRC) $(UTILS_SRC) $(REGISTRY_SRC) $(HASH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(NCURSES_FLAGS) $(LDFLAGS)

$(SERVER_BIN): $(SERVER_SRC) $(UTILS_SRC) $(REGISTRY_SRC) $(HASH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

clean:
	rm -rf $(BUILD_DIR)
