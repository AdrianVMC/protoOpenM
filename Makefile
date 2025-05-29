CC = gcc
CFLAGS = -Iinclude
LDFLAGS = -lncurses
CLIENT_SRC = client/cli.c utils/file_utils.c
SERVER_SRC = server/server.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_TARGET = openMS
SERVER_TARGET = openMS-server

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(CC) -o $(CLIENT_TARGET) $(CLIENT_OBJ) $(LDFLAGS)

$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) -o $(SERVER_TARGET) $(SERVER_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_TARGET) $(SERVER_TARGET)
