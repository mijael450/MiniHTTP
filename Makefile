CC      = gcc
CFLAGS  = -Wall -Wextra -g -I include
SRCS    = src/main.c src/server.c src/http.c src/mime.c src/files.c
TARGET  = minihttpd

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)