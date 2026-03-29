#include <string.h>

#include "unity.h"

#include "../HttpParser.h"
#include "../Log.h"

void setUp( void ) {}
void tearDown( void ) {}

//------------------------------------------------------------------------------
// Helper: build a raw HTTP request string and return its total length
static int build_request( char* buf, const char* raw )
{
   int len = strlen( raw );
   memcpy( buf, raw, len );
   return len;
}

//------------------------------------------------------------------------------
// Helper: assert that a sand_string_view_t equals a C-string
static void assert_string_view_equal( const char* expected,
                                      sand_string_view_t view )
{
   TEST_ASSERT_EQUAL( strlen( expected ), view.size );
   TEST_ASSERT_EQUAL_MEMORY( expected, view.data, view.size );
}

// ===== Existing-functionality tests (should pass) =====

//------------------------------------------------------------------------------
void test_parse_valid_get_request( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET /index.html HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_GET, req.method_int );
   assert_string_view_equal( "/index.html", req.uri_view );
   TEST_ASSERT_EQUAL( 1001, req.version_int );
}

//------------------------------------------------------------------------------
void test_parse_multiple_headers( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Host: example.com\r\n"
                                  "Accept: text/html\r\n"
                                  "Connection: keep-alive\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "host", req.headers[ 0 ].name );
   assert_string_view_equal( "example.com", req.headers[ 0 ].value );
   TEST_ASSERT_EQUAL_STRING( "accept", req.headers[ 1 ].name );
   assert_string_view_equal( "text/html", req.headers[ 1 ].value );
   TEST_ASSERT_EQUAL_STRING( "connection", req.headers[ 2 ].name );
   assert_string_view_equal( "keep-alive", req.headers[ 2 ].value );
}

//------------------------------------------------------------------------------
void test_headers_stored_lowercase( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "content-type", req.headers[ 1 ].name );
}

//------------------------------------------------------------------------------
void test_header_without_colon_returns_error( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "BadHeaderNoColon\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_ERROR_INVALID_HEADERS, res );
}

//------------------------------------------------------------------------------
// HTTP/1.1 with no headers at all — must fail because Host is missing
void test_request_with_no_headers_http11( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_ERROR_MISSING_HOST, res );
   TEST_ASSERT_EQUAL( 0, req.header_count );
}

// HTTP/1.0 with no headers — should be fine (no Host required)
void test_request_with_no_headers_http10( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.0\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( 0, req.header_count );
}

//------------------------------------------------------------------------------
void test_parse_post_with_content_length( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "POST /data HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Content-Length: 13\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_POST, req.method_int );
   assert_string_view_equal( "/data", req.uri_view );
   TEST_ASSERT_EQUAL_STRING( "content-length", req.headers[ 1 ].name );
   assert_string_view_equal( "13", req.headers[ 1 ].value );
}

// ===== TDD tests for HTTP/1.1 compliance (may fail until implemented) =====

//------------------------------------------------------------------------------
// HTTP/1.1 requires Host header (RFC 7230 section 5.4)
// A server MUST respond with 400 if Host is missing in HTTP/1.1
void test_http11_missing_host_header_should_error( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Accept: text/html\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_NOT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( PARSE_ERROR_MISSING_HOST, res );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1 must handle chunked Transfer-Encoding (RFC 7230 section 3.3.1)
void test_http11_chunked_transfer_encoding_header( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "POST /upload HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Transfer-Encoding: chunked\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "transfer-encoding", req.headers[ 1 ].name );
   assert_string_view_equal( "chunked", req.headers[ 1 ].value );

   // TODO: actual chunked body parsing is not yet implemented
   TEST_IGNORE_MESSAGE( "TODO: Implement chunked transfer-encoding body parsing" );
}

//------------------------------------------------------------------------------
// HTTP/1.1 header field values can span multiple lines via obs-fold (deprecated but must handle)
// RFC 7230 section 3.2.4: obs-fold = CRLF 1*( SP / HTAB )
void test_http11_obs_fold_multiline_header( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // obs-fold: continuation line starts with SP or HTAB
   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "X-Long-Header: value1\r\n"
                                  " continued-value\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   // Once implemented, the folded value should be merged:
   // TEST_ASSERT_EQUAL( PARSE_OK, res );
   // TEST_ASSERT_EQUAL_STRING( "value1 continued-value", req.headers[ 1 ].value );
   TEST_IGNORE_MESSAGE( "TODO: Handle obs-fold (deprecated line folding) in headers" );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: must parse request with absolute-form URI (RFC 7230 section 5.3.2)
// e.g. GET http://www.example.com/path HTTP/1.1
// The parser state machine goes: sand_schema -> sand_schema_slash ->
// sand_schema_slash_slash -> sand_host_start (needs implementation)
// sand_host_start must skip the authority (host[:port]) and land on the path
void test_http11_absolute_form_uri( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET http://www.example.com/index.html HTTP/1.1\r\n"
                                  "Host: www.example.com\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_GET, req.method_int );
   // The extracted path should be just the path portion, not the full URI
   assert_string_view_equal( "/index.html", req.uri_view );
   TEST_ASSERT_EQUAL( 1001, req.version_int );
}

//------------------------------------------------------------------------------
// HTTP/1.1: absolute-form URI with no trailing path defaults to "/"
// e.g. GET http://www.example.com HTTP/1.1 -> path should be "/"
void test_http11_absolute_form_uri_no_path( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET http://www.example.com HTTP/1.1\r\n"
                                  "Host: www.example.com\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_GET, req.method_int );
   // When no path follows the authority, default to "/"
   assert_string_view_equal( "/", req.uri_view );
   TEST_ASSERT_EQUAL( 1001, req.version_int );
}

//------------------------------------------------------------------------------
// HTTP/1.1: absolute-form URI with https scheme
void test_http11_absolute_form_uri_https( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET https://secure.example.com/login HTTP/1.1\r\n"
                                  "Host: secure.example.com\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_GET, req.method_int );
   assert_string_view_equal( "/login", req.uri_view );
   TEST_ASSERT_EQUAL( 1001, req.version_int );
}

//------------------------------------------------------------------------------
// HTTP/1.1: absolute-form URI with port number in authority
// e.g. GET http://example.com:8080/api/data HTTP/1.1
void test_http11_absolute_form_uri_with_port( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET http://example.com:8080/api/data HTTP/1.1\r\n"
                                  "Host: example.com:8080\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_GET, req.method_int );
   assert_string_view_equal( "/api/data", req.uri_view );
}

//------------------------------------------------------------------------------
// HTTP/1.1: absolute-form URI with port but no trailing path
// e.g. GET http://example.com:3000 HTTP/1.1 -> path should be "/"
void test_http11_absolute_form_uri_with_port_no_path( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET http://example.com:3000 HTTP/1.1\r\n"
                                  "Host: example.com:3000\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_GET, req.method_int );
   assert_string_view_equal( "/", req.uri_view );
}

//------------------------------------------------------------------------------
// HTTP/1.1: absolute-form URI with deep nested path
void test_http11_absolute_form_uri_deep_path( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "POST http://api.example.com/v1/users/42/profile HTTP/1.1\r\n"
                                  "Host: api.example.com\r\n"
                                  "Content-Length: 0\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_POST, req.method_int );
   assert_string_view_equal( "/v1/users/42/profile", req.uri_view );
}

//------------------------------------------------------------------------------
// HTTP/1.1: absolute-form URI with query string
void test_http11_absolute_form_uri_with_query( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET http://example.com/search?q=hello&lang=en HTTP/1.1\r\n"
                                  "Host: example.com\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_GET, req.method_int );
   // Path + query string should be preserved
   assert_string_view_equal( "/search?q=hello&lang=en", req.uri_view );
}

//------------------------------------------------------------------------------
// HTTP/1.1: support persistent connections by default (RFC 7230 section 6.3)
// Connection: close means server should close after response
void test_http11_connection_close_header( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Connection: close\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "connection", req.headers[ 1 ].name );
   assert_string_view_equal( "close", req.headers[ 1 ].value );

   // TODO: server should track connection persistence state
   TEST_IGNORE_MESSAGE( "TODO: Track connection persistence (keep-alive vs close)" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: must handle HEAD method (RFC 7231 section 4.3.2)
void test_parse_head_request( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "HEAD /index.html HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_UNKNOWN, req.method_int );
   assert_string_view_equal( "/index.html", req.uri_view );
}

//------------------------------------------------------------------------------
// HTTP/1.1: must handle PUT method (RFC 7231 section 4.3.4)
void test_parse_put_request( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "PUT /resource HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Content-Length: 5\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_PUT, req.method_int );
}

//------------------------------------------------------------------------------
// HTTP/1.1: must handle DELETE method (RFC 7231 section 4.3.5)
void test_parse_delete_request( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "DELETE /resource/42 HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( SAND_HTTP_DELETE, req.method_int );
   assert_string_view_equal( "/resource/42", req.uri_view );
}

//------------------------------------------------------------------------------
// HTTP/1.1: whitespace handling in header values (RFC 7230 section 3.2.6)
// Leading/trailing whitespace in header values should be stripped
void test_http11_header_value_whitespace_trimming( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Header value with extra leading/trailing whitespace
   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "      Host:   localhost   \r\n"
                                  "Accept:  text/html \r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   assert_string_view_equal( "localhost", req.headers[ 0 ].value );
   assert_string_view_equal( "text/html", req.headers[ 1 ].value );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 400 Bad Request for malformed request line (RFC 7230 section 3.1.1)
// Missing method, path, or version should be rejected
void test_http11_malformed_request_line_missing_version( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Request line missing version
   int len = build_request( buf, "GET /index.html\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_ERROR_MALFORMED_REQUEST_LINE, res );
   (void)res;
}

//------------------------------------------------------------------------------
void test_http11_malformed_request_line_missing_method( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };
   //[WARN]  method: ex.html path: HTTP/1.1 version: Host: | HttpParser.c:25

   // Request line missing version
   int len = build_request( buf, "/index.html  HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_ERROR_MALFORMED_REQUEST_LINE, res );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: 400 Bad Request for empty request line
void test_http11_empty_request_line( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Completely empty — just the terminator
   int len = build_request( buf, "\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   // Once implemented:
   TEST_ASSERT_EQUAL( PARSE_ERROR_MALFORMED_REQUEST_LINE, res );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: 414 URI Too Long (RFC 7231 section 6.5.12)
// Server should reject requests with excessively long URIs
void test_http11_uri_too_long( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Build a path longer than 255 chars (req.path is char[256])
   char long_uri[ 300 ];
   long_uri[ 0 ] = '/';
   memset( long_uri + 1, 'a', 298 );
   long_uri[ 299 ] = '\0';

   char raw[ 4096 ];
   snprintf( raw, sizeof( raw ), "GET %s HTTP/1.1\r\nHost: localhost\r\n\r\n", long_uri );

   int len = build_request( buf, raw );
   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_ERROR_PATH_TOO_LONG, res );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: 431 Request Header Fields Too Large (RFC 6585 section 5)
// Reject requests when individual header or total headers are too large
void test_http11_header_fields_too_large( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Header name longer than MAX_HEADER_NAME_LEN (64)
   char long_name[ 80 ];
   memset( long_name, 'X', 79 );
   long_name[ 79 ] = '\0';

   char raw[ 4096 ];
   snprintf( raw, sizeof( raw ),
             "GET / HTTP/1.1\r\nHost: localhost\r\n%s: value\r\n\r\n", long_name );

   int len = build_request( buf, raw );
   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_NOT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( PARSE_ERROR_HEADER_NAME_TOO_LONG, res );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: reject conflicting Content-Length and Transfer-Encoding (RFC 7230 section 3.3.3)
// If both are present, Transfer-Encoding takes precedence, Content-Length must be removed
void test_http11_conflicting_content_length_and_transfer_encoding( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Both Content-Length and Transfer-Encoding present — must reject or
   // ignore Content-Length per RFC 7230 section 3.3.3
   int len = build_request( buf, "POST /data HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Content-Length: 10\r\n"
                                  "Transfer-Encoding: chunked\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   // Once implemented, Transfer-Encoding takes precedence:
   TEST_ASSERT_EQUAL( PARSE_OK, res );
   // Content-Length should be ignored/removed when Transfer-Encoding is present
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: 411 Length Required (RFC 7231 section 6.5.10)
// POST/PUT without Content-Length or Transfer-Encoding should get 411
void test_http11_length_required_for_body( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // POST without Content-Length or Transfer-Encoding
   int len = build_request( buf, "POST /data HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   // Parser parses headers fine, but the server layer should detect
   // that a POST has no length indication and respond 411
   // This could be a parser-level check or server-level check
   // TEST_ASSERT_EQUAL( PARSE_OK, res );  // or a new error code
   TEST_IGNORE_MESSAGE( "TODO: Detect missing Content-Length/Transfer-Encoding on requests that need a body" );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.0: no Host header required, connection closes by default
void test_http10_no_host_required( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.0\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( 1000, req.version_int );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 505 HTTP Version Not Supported (RFC 7231 section 6.6.6)
void test_http11_unsupported_version( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/2.0\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   // Currently parses fine — version is just stored as a string
   // Once implemented, should reject unsupported versions:
   TEST_ASSERT_NOT_EQUAL( PARSE_OK, res );
   //TEST_IGNORE_MESSAGE( "TODO: Return error for unsupported HTTP versions (e.g. HTTP/2.0 in plaintext)" );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: multiple Content-Length values must all agree (RFC 7230 section 3.3.2)
void test_http11_multiple_content_length_must_agree( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Two Content-Length headers with different values — must reject
   int len = build_request( buf, "POST /data HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "Content-Length: 10\r\n"
                                  "Content-Length: 20\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   // Once implemented:
   // TEST_ASSERT_NOT_EQUAL( PARSE_OK, res );
   TEST_IGNORE_MESSAGE( "TODO: Reject request if multiple Content-Length headers disagree" );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: reject header names with spaces before colon (RFC 7230 section 3.2.4)
void test_http11_reject_space_before_colon_in_header( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Space before colon is invalid per RFC 7230
   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Host : localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   // Once implemented:
   TEST_ASSERT_EQUAL( PARSE_ERROR_INVALID_HEADERS, res );
   //TEST_IGNORE_MESSAGE( "TODO: Reject header lines with whitespace between name and colon" );
   (void)res;
}

//------------------------------------------------------------------------------
// HTTP/1.1: handle MAX_HEADERS overflow gracefully
void test_http11_too_many_headers( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   // Build a request with MAX_HEADERS + 1 headers (including Host)
   char raw[ 4096 ];
   int  off = 0;
   off += snprintf( raw + off, sizeof( raw ) - off, "GET / HTTP/1.1\r\n" );
   off += snprintf( raw + off, sizeof( raw ) - off, "Host: localhost\r\n" );
   for ( int i = 0; i < MAX_HEADERS; i++ )
   {
      off += snprintf( raw + off, sizeof( raw ) - off, "X-H%d: val%d\r\n", i, i );
   }
   off += snprintf( raw + off, sizeof( raw ) - off, "\r\n" );

   int len = build_request( buf, raw );
   ParseResult_t res = http_parser_parse_request( buf, len, &req );

   TEST_ASSERT_NOT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL( MAX_HEADERS, req.header_count );
   (void)res;
}

//------------------------------------------------------------------------------
int main( void )
{
   UNITY_BEGIN();

   // Existing functionality
   RUN_TEST( test_parse_valid_get_request );
   RUN_TEST( test_parse_multiple_headers );
   RUN_TEST( test_headers_stored_lowercase );
   RUN_TEST( test_header_without_colon_returns_error );
   RUN_TEST( test_request_with_no_headers_http11 );
   RUN_TEST( test_request_with_no_headers_http10 );
   RUN_TEST( test_parse_post_with_content_length );

   // HTTP/1.1 compliance (TDD - some will be IGNORED until implemented)
   RUN_TEST( test_http11_missing_host_header_should_error );
   RUN_TEST( test_http11_chunked_transfer_encoding_header );
   RUN_TEST( test_http11_obs_fold_multiline_header );
   RUN_TEST( test_http11_absolute_form_uri );
   RUN_TEST( test_http11_absolute_form_uri_no_path );
   RUN_TEST( test_http11_absolute_form_uri_https );
   RUN_TEST( test_http11_absolute_form_uri_with_port );
   RUN_TEST( test_http11_absolute_form_uri_with_port_no_path );
   RUN_TEST( test_http11_absolute_form_uri_deep_path );
   RUN_TEST( test_http11_absolute_form_uri_with_query );
   RUN_TEST( test_http11_connection_close_header );
   RUN_TEST( test_parse_head_request );
   RUN_TEST( test_parse_put_request );
   RUN_TEST( test_parse_delete_request );
   RUN_TEST( test_http11_header_value_whitespace_trimming );
   RUN_TEST( test_http11_malformed_request_line_missing_version );
   RUN_TEST( test_http11_malformed_request_line_missing_method );
   RUN_TEST( test_http11_empty_request_line );
   RUN_TEST( test_http11_uri_too_long );
   RUN_TEST( test_http11_header_fields_too_large );
   RUN_TEST( test_http11_conflicting_content_length_and_transfer_encoding );
   RUN_TEST( test_http11_length_required_for_body );
   RUN_TEST( test_http10_no_host_required );
   RUN_TEST( test_http11_unsupported_version );
   RUN_TEST( test_http11_multiple_content_length_must_agree );
   RUN_TEST( test_http11_reject_space_before_colon_in_header );
   RUN_TEST( test_http11_too_many_headers );

   return UNITY_END();
}
