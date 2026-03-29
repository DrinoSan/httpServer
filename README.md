# httpServer

A lightweight HTTP/1.1 server written in C11 using an event-driven architecture (kqueue) on macOS.
This Server is more of a learning project to understand the low level aspects of a http server. But the goal is to make it usable at least for testing.
Also important to mention as a go to resource I use NGINX first of all to learn C better and also NGINX is the GOAT for http servers :)

## Supported Features

- **HTTP Versions**: HTTP/0.9, HTTP/1.0, HTTP/1.1
- **Methods**: GET, HEAD, POST, PUT, DELETE, PATCH, OPTIONS, TRACE, CONNECT (+ WebDAV methods)
- **Header Parsing**: up to 32 headers per request, case-insensitive names, whitespace trimming, RFC 7230 validation
- **Host Header Enforcement**: required for HTTP/1.1 as per spec
- **Content-Length**: request body reading supported
- **Absolute-form URIs**: `GET http://example.com/path HTTP/1.1` handled correctly
- **Routing**: exact-match router with method + path lookup, 404 fallback
- **Response Serialization**: status line, headers, body with auto Content-Length
- **Multi-threaded Workers**: 5 worker threads with round-robin connection distribution
- **Event-driven I/O**: non-blocking via kqueue with read and timer filters

## Building

```
make          # build the server
make test     # build and run unit tests
make clean    # clean build artifacts
```

## Unit Tests

The project uses the [Unity](https://github.com/ThrowTheSwitch/Unity) C test framework. Four test suites cover the core components:

- `test_http_parser` — request line and header parsing (methods, versions, edge cases, error handling)
- `test_router` — route registration, lookup, method matching, 404 handling
- `test_http_request` — header lookup, string view handling
- `test_http_response` — response serialization, status codes, Content-Length calculation

## Architecture
- The main data structure is Connection_t which is created for a new request. The Connection_t holds a buffer currently ~8000 byte.
This buffer is used as source for the string views. The httpParser holds pointers like method_start, method_end, uri_start, uri_end and others which point to the buffer.
Those pointers are part of the HttpRequest_t. Therefore it is important to understand that the connection is the owner of all data.
  - What is not using pointers are the header names in the httpRequest. This decision was made because I lowercase the header names. The values on the other hand are indeed just pointers to the connection buffer


## NGINX
- The httpParser uses a state machine for parsing the request line which is also similiar to how NGINX does it.
A big difference is, that NGINX uses a more performant way of parsing.. as soon as something is read from the socket the state machine continues parsing and remembers its last state to continue parsing.
This has the advantage, that as soon as a parsing error occures the request is rejected. In contrast my server just casually reads the full request( Reading until \r\n\r\n or timeout ).
And only then I start parsing, this can later be changed but for now its okay enough.
