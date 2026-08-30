# httpServer

A lightweight HTTP/1.1 server written in C11 using an event-driven architecture with kqueue on macOS. Built as a learning project to understand low-level HTTP server internals, inspired by NGINX's design patterns. The goal is to make it usable for small projects and local testing.

## Features

### HTTP Protocol
- **Versions**: HTTP/0.9, HTTP/1.0, HTTP/1.1
- **Methods**: GET, HEAD, POST, PUT, DELETE, PATCH, OPTIONS, TRACE, CONNECT (+ WebDAV: MKCOL, COPY, MOVE, PROPFIND, PROPPATCH, LOCK, UNLOCK)
- **Header parsing**: up to 32 headers per request, names lowercased automatically, whitespace trimming, RFC 7230 validation
- **Host header enforcement**: required for HTTP/1.1 (responds 400 if missing)
- **Content-Length**: request body reading with timeout
- **Absolute-form URIs**: `GET http://example.com/path HTTP/1.1` handled correctly
- **Error responses**: 400 Bad Request, 404 Not Found, 405 Method Not Allowed (with Allow header), 414 URI Too Long, 431 Header Fields Too Large, 501 Not Implemented

### Server Architecture
- **Event-driven I/O**: non-blocking via kqueue with read and timer filters
- **Multi-threaded**: 5 worker threads with round-robin connection distribution
- **Connection state machine**: READING_HEADERS -> READING_BODY -> SENDING_RESPONSE
- **8 KB per-connection buffer**: zero-copy header parsing using string views

### Routing
- Exact-match router with method + path lookup
- Up to 32 registered routes
- Built-in fallback handlers for 404, 405, and 501

## Quick Start

### Build

```bash
git clone --recursive <repo-url>
cd httpServer
make
```

### Run

```bash
./httpServer
# Server starts on port 8080
curl http://localhost:8080/home
```

### Test

```bash
make test     # build and run all unit tests
make clean    # clean build artifacts
```

## Usage Example

```c
#include "Server.h"
#include "Connection.h"
#include "Router.h"

void handle_index(Connection_t* con)
{
    con->response.status_code = 200;
    strcpy(con->response.status_text, "OK");
    con->response.body = "<h1>Hello, World!</h1>";
}

void handle_api_users(Connection_t* con)
{
    con->response.status_code = 200;
    strcpy(con->response.status_text, "OK");
    con->response.body = "{\"users\": []}";
}

int main()
{
    Server_t server;
    server_create(&server);

    router_add_route(&server.router, SAND_HTTP_GET, "/", handle_index);
    router_add_route(&server.router, SAND_HTTP_GET, "/api/users", handle_api_users);
    router_add_route(&server.router, SAND_HTTP_POST, "/api/users", handle_api_users);

    server_start(&server);   // blocks — runs the event loop

    server_destroy(&server);
}
```

## Architecture

```
┌──────────────────────────────────────────────────┐
│                    main()                        │
│   server_create() -> router_add_route() x N      │
│   server_start()  [blocking event loop]          │
└────────────────────────┬─────────────────────────┘
                         │ accept()
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
   ┌───────-──┐     ┌──────-───┐     ┌──────-───┐
   │ Worker 1 │     │ Worker 2 │ ... │ Worker 5 │
   │ (kqueue) │     │ (kqueue) │     │ (kqueue) │
   └────┬─────┘     └────┬─────┘     └────┬─────┘
        │                │                │
        └────────────────┼────────────────┘
                         │
           ┌─────────────▼──────────────┐
           │  Per-connection pipeline:   │
           │  1. Read into 8 KB buffer   │
           │  2. HttpParser (state machine)│
           │  3. Router lookup           │
           │  4. Handler callback        │
           │  5. Serialize response      │
           │  6. Send & close            │
           └────────────────────────────┘
```

### Modules

| Module | Responsibility |
|--------|---------------|
| `Server.c/h` | Event loop, worker threads, connection acceptance |
| `SocketHandler.c/h` | Socket creation, binding, listening, accepting |
| `Connection.c/h` | Per-connection state (buffer, request, response, state machine) |
| `HttpParser.c/h` | NGINX-style state machine for request line + header parsing |
| `HttpRequest.c/h` | Parsed request data structure, header lookup |
| `HttpResponse.c/h` | Response serialization (status line, Content-Length, body) |
| `Router.c/h` | Exact-match routing, method + path, fallback handlers |
| `Log.c/h` | Color-coded logging with file/line info |
| `HttpHeader.h` | Header structure (name + string view value) |

### Key Design Decisions

- **Connection owns all data**: `Connection_t` holds an 8 KB buffer. The parser stores pointers (string views) into this buffer rather than copying, minimizing allocations.
- **Header names are copied and lowercased**: since RFC 7230 says header names are case-insensitive, lowercasing at parse time simplifies lookup. Values remain as string views into the connection buffer.
- **NGINX-style state machine**: the parser processes the request line byte-by-byte with explicit state transitions, similar to NGINX's `ngx_http_parse_request_line`. This enables future incremental parsing.
- **Method bitmask**: each HTTP method is a power-of-2 integer, enabling fast bitwise validation (`method & ~SAND_HTTP_ALL_METHODS` detects invalid methods).

## API Reference

### Server
```c
void server_create(Server_t* server);     // Initialize server
void server_start(Server_t* server);      // Start event loop (blocking)
void server_destroy(Server_t* server);    // Cleanup
```

### Router
```c
void router_add_route(Router_t* router, int32_t method, const char* path, RouteHandler_t handler);
RouteHandler_t router_find_route(Router_t* router, HttpRequest_t* request);
```

### Handler Signature
```c
typedef void (*RouteHandler_t)(Connection_t* con);
```

Inside a handler, set the response fields on `con->response`:
```c
void my_handler(Connection_t* con)
{
    con->response.status_code = 200;
    strcpy(con->response.status_text, "OK");
    con->response.body = "<h1>Hello</h1>";
}
```

### Request Inspection
```c
// Access parsed request inside a handler:
con->request.method_int   // SAND_HTTP_GET, SAND_HTTP_POST, etc.
con->request.uri_view     // sand_string_view_t with path
con->request.version_int  // 9, 1000, or 1001
con->request.body         // pointer to body data (if Content-Length > 0)
con->request.content_length

// Find a header value (names are lowercased):
const sand_string_view_t* val = http_request_find_header(&con->request, "content-type");
```

### HTTP Method Constants
```c
SAND_HTTP_GET       0x00000002
SAND_HTTP_HEAD      0x00000004
SAND_HTTP_POST      0x00000008
SAND_HTTP_PUT       0x00000010
SAND_HTTP_DELETE    0x00000020
SAND_HTTP_OPTIONS   0x00000200
SAND_HTTP_PATCH     0x00004000
```

## Unit Tests

The project uses the [Unity](https://github.com/ThrowTheSwitch/Unity) C test framework. Four test suites cover the core components:

| Suite | Description |
|-------|-------------|
| `test_http_parser` | Request line parsing, header parsing, methods, versions, absolute URIs, error handling |
| `test_router` | Route registration, lookup, method matching, 404/405/501 fallbacks |
| `test_http_request` | Header lookup, string view handling, missing headers |
| `test_http_response` | Response serialization, status codes, Content-Length, body handling |

## Limitations

- **macOS only**: uses kqueue (no epoll/IOCP support)
- **No TLS/HTTPS**: plaintext HTTP only
- **No keep-alive**: connections close after each response
- **No chunked encoding**: Transfer-Encoding: chunked is parsed but not decoded

## Dependencies

- C11 compiler (gcc/clang)
- pthread
- [sandlib](https://github.com/) — custom string utilities (git submodule)
- [Unity](https://github.com/ThrowTheSwitch/Unity) — C test framework (git submodule)

## Comparison with NGINX

The HTTP parser uses a state machine similar to NGINX's `ngx_http_parse_request_line`. A key difference: NGINX parses incrementally as bytes arrive from the socket, rejecting malformed requests immediately. This server reads the complete request (until `\r\n\r\n` or timeout) before parsing, which is simpler but less efficient. This can be changed later without altering the parser API.
