CC=gcc
CFLAGS=-Iinclude

TARGET=openMS

all: $(TARGET)

$(TARGET): client/cli.o utils/file_utils.o
	$(CC) -o $(TARGET) client/cli.o utils/file_utils.o

client/cli.o: client/cli.c
	$(CC) $(CFLAGS) -c client/cli.c -o client/cli.o

utils/file_utils.o: utils/file_utils.c
	$(CC) $(CFLAGS) -c utils/file_utils.c -o utils/file_utils.o

clean:
	rm -f *.o $(TARGET)
