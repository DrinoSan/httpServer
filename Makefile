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
	rm -f $(OBJS) $(TARGET) tests/test_http_parser tests/test_router tests/test_http_request tests/test_http_response

# --- Test targets ---
UNITY_SRC   = unity/src/unity.c
TEST_CFLAGS = $(CFLAGS) -Iunity/src -DUNITY_OUTPUT_COLOR

tests/test_http_parser: tests/test_http_parser.c HttpParser.c Log.c $(UNITY_SRC) $(SANDLIB)
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_http_parser.c HttpParser.c Log.c $(UNITY_SRC) -Lsandlib -lsand

tests/test_router: tests/test_router.c Router.c Log.c $(UNITY_SRC) $(SANDLIB)
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_router.c Router.c Log.c $(UNITY_SRC) -Lsandlib -lsand

tests/test_http_request: tests/test_http_request.c HttpRequest.c $(UNITY_SRC) $(SANDLIB)
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_http_request.c HttpRequest.c $(UNITY_SRC) -Lsandlib -lsand

tests/test_http_response: tests/test_http_response.c HttpResponse.c $(UNITY_SRC) $(SANDLIB)
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_http_response.c HttpResponse.c $(UNITY_SRC) -Lsandlib -lsand

test: $(SANDLIB) tests/test_http_parser tests/test_router tests/test_http_request tests/test_http_response
	@echo "=== Running test_http_parser ==="
	./tests/test_http_parser
	@echo ""
	@echo "=== Running test_router ==="
	./tests/test_router
	@echo ""
	@echo "=== Running test_http_request ==="
	./tests/test_http_request
	@echo ""
	@echo "=== Running test_http_response ==="
	./tests/test_http_response

.PHONY: all clean test
