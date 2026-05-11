CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -g
# Se añade -ldaemon para que el linker encuentre la librería
LDFLAGS = -ldaemon

SRCS = main.c arguments.c server.c utils.c pi.c dtp.c handlers.c signals.c session.c log.c
OBJS = $(SRCS:.c=.o)
TARGET = miniftp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

