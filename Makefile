# Makefile para protoOpenM con registro automático y ncurses

CC = gcc
CFLAGS = -Wall -pthread -Iinclude

NCURSES_FLAGS = -L/opt/homebrew/opt/ncurses/lib -lncurses -I/opt/homebrew/opt/ncurses/include

BUILD_DIR = build

CLIENT_SRC = client/cli.c
SERVER_SRC = server/server.c
UTILS_SRC = utils/shared_utils.c
REGISTRY_SRC = utils/client_registry.c

CLIENT_BIN = $(BUILD_DIR)/client
SERVER_BIN = $(BUILD_DIR)/server

.PHONY: all clean run-client run-server

all: $(CLIENT_BIN) $(SERVER_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CLIENT_BIN): $(CLIENT_SRC) $(UTILS_SRC) $(REGISTRY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(NCURSES_FLAGS) -o $@ $^

$(SERVER_BIN): $(SERVER_SRC) $(UTILS_SRC) $(REGISTRY_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

clean:
	rm -rf $(BUILD_DIR)
