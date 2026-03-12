CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11
LDFLAGS  =

TARGET   = httpServer
SRCS     = main.c Server.c SocketHandler.c
OBJS     = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
