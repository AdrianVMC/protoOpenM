CC = gcc
CFLAGS = -Iinclude
LDFLAGS = -lncurses
SRC = client/cli.c utils/file_utils.c
OBJ = $(SRC:.c=.o)
TARGET = openMS

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
