CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -Isandlib
LDFLAGS  = -lpthread

TARGET   = httpServer
SRCS     = main.c Server.c SocketHandler.c Connection.c HttpParser.c HttpRequest.c Log.c Router.c HttpResponse.c
OBJS     = $(SRCS:.c=.o)

SANDLIB  = sandlib/libsand.a

all: $(TARGET)

$(SANDLIB):
	$(MAKE) -C sandlib

$(TARGET): $(OBJS) $(SANDLIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -Lsandlib -lsand $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
