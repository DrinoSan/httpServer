#include <string.h>

#include "unity.h"

#include "../HttpParser.h"

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

// ===== Existing-functionality tests (should pass) =====

//------------------------------------------------------------------------------
void test_parse_valid_get_request( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET /index.html HTTP/1.1\r\n"
                                  "Host: localhost\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "GET", req.method );
   TEST_ASSERT_EQUAL_STRING( "/index.html", req.path );
   TEST_ASSERT_EQUAL_STRING( "HTTP/1.1", req.version );
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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "host", req.headers[ 0 ].name );
   TEST_ASSERT_EQUAL_STRING( "example.com", req.headers[ 0 ].value );
   TEST_ASSERT_EQUAL_STRING( "accept", req.headers[ 1 ].name );
   TEST_ASSERT_EQUAL_STRING( "text/html", req.headers[ 1 ].value );
   TEST_ASSERT_EQUAL_STRING( "connection", req.headers[ 2 ].name );
   TEST_ASSERT_EQUAL_STRING( "keep-alive", req.headers[ 2 ].value );
}

//------------------------------------------------------------------------------
void test_headers_stored_lowercase( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "content-type", req.headers[ 0 ].name );
}

//------------------------------------------------------------------------------
void test_header_without_colon_returns_error( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "BadHeaderNoColon\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_ERROR_INVALID_HEADERS, res );
}

//------------------------------------------------------------------------------
// This test documents the expected correct behavior: 0 headers parsed -> count=0
void test_request_with_no_headers( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.1\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "POST", req.method );
   TEST_ASSERT_EQUAL_STRING( "/data", req.path );
   TEST_ASSERT_EQUAL_STRING( "content-length", req.headers[ 1 ].name );
   TEST_ASSERT_EQUAL_STRING( "13", req.headers[ 1 ].value );
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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   // For now the parser doesn't validate Host requirement
   // Once implemented, this should return an error for HTTP/1.1 without Host
   TEST_IGNORE_MESSAGE( "TODO: HTTP/1.1 requires Host header validation" );
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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "transfer-encoding", req.headers[ 1 ].name );
   TEST_ASSERT_EQUAL_STRING( "chunked", req.headers[ 1 ].value );

   // TODO: actual chunked body parsing is not yet implemented
   TEST_IGNORE_MESSAGE( "TODO: Implement chunked transfer-encoding body parsing" );
}

//------------------------------------------------------------------------------
// HTTP/1.1 header field values can span multiple lines via obs-fold (deprecated but must handle)
// RFC 7230 section 3.2.4: obs-fold = CRLF 1*( SP / HTAB )
void test_http11_obs_fold_multiline_header( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Handle obs-fold (deprecated line folding) in headers" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: must parse request with absolute-form URI (RFC 7230 section 5.3.2)
// e.g. GET http://www.example.com/path HTTP/1.1
void test_http11_absolute_form_uri( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Parse absolute-form URI in request line" );
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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "connection", req.headers[ 1 ].name );
   TEST_ASSERT_EQUAL_STRING( "close", req.headers[ 1 ].value );

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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "HEAD", req.method );
   TEST_ASSERT_EQUAL_STRING( "/index.html", req.path );
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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "PUT", req.method );
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

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "DELETE", req.method );
   TEST_ASSERT_EQUAL_STRING( "/resource/42", req.path );
}

//------------------------------------------------------------------------------
// HTTP/1.1: whitespace handling in header values (RFC 7230 section 3.2.6)
// Leading/trailing whitespace in header values should be stripped
void test_http11_header_value_whitespace_trimming( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Trim leading/trailing OWS from header values" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 400 Bad Request for malformed request line (RFC 7230 section 3.1.1)
// Missing method, path, or version should be rejected
void test_http11_malformed_request_line_missing_version( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Return PARSE_ERROR_MALFORMED_REQUEST_LINE for incomplete request line" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 400 Bad Request for empty request line
void test_http11_empty_request_line( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Return error for empty request line" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 414 URI Too Long (RFC 7231 section 6.5.12)
// Server should reject requests with excessively long URIs
void test_http11_uri_too_long( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Return PARSE_ERROR_PATH_TOO_LONG for URIs exceeding limit" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 431 Request Header Fields Too Large (RFC 6585 section 5)
// Reject requests when individual header or total headers are too large
void test_http11_header_fields_too_large( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Reject requests with header fields exceeding MAX_HEADER_NAME_LEN/MAX_HEADER_VALUE_LEN" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: reject conflicting Content-Length and Transfer-Encoding (RFC 7230 section 3.3.3)
// If both are present, Transfer-Encoding takes precedence, Content-Length must be removed
void test_http11_conflicting_content_length_and_transfer_encoding( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Handle conflicting Content-Length and Transfer-Encoding headers" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 411 Length Required (RFC 7231 section 6.5.10)
// POST/PUT without Content-Length or Transfer-Encoding should get 411
void test_http11_length_required_for_body( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Detect missing Content-Length/Transfer-Encoding on requests that need a body" );
}

//------------------------------------------------------------------------------
// HTTP/1.0: no Host header required, connection closes by default
void test_http10_no_host_required( void )
{
   char           buf[ 4096 ];
   HttpRequest_t  req = { 0 };

   int len = build_request( buf, "GET / HTTP/1.0\r\n"
                                  "\r\n" );

   ParseResult_t res = http_parser_parse_headers( buf, len, &req );

   TEST_ASSERT_EQUAL( PARSE_OK, res );
   TEST_ASSERT_EQUAL_STRING( "HTTP/1.0", req.version );

   // HTTP/1.0 should not require Host header
   TEST_IGNORE_MESSAGE( "TODO: HTTP/1.0 should not require Host header; connection defaults to close" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 505 HTTP Version Not Supported (RFC 7231 section 6.6.6)
void test_http11_unsupported_version( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Return error for unsupported HTTP versions (e.g. HTTP/2.0 in plaintext)" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: multiple Content-Length values must all agree (RFC 7230 section 3.3.2)
void test_http11_multiple_content_length_must_agree( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Reject request if multiple Content-Length headers disagree" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: reject header names with spaces before colon (RFC 7230 section 3.2.4)
void test_http11_reject_space_before_colon_in_header( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Reject header lines with whitespace between name and colon" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: handle MAX_HEADERS overflow gracefully
void test_http11_too_many_headers( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Return error when request exceeds MAX_HEADERS count" );
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
   RUN_TEST( test_request_with_no_headers );
   RUN_TEST( test_parse_post_with_content_length );

   // HTTP/1.1 compliance (TDD - some will be IGNORED until implemented)
   RUN_TEST( test_http11_missing_host_header_should_error );
   RUN_TEST( test_http11_chunked_transfer_encoding_header );
   RUN_TEST( test_http11_obs_fold_multiline_header );
   RUN_TEST( test_http11_absolute_form_uri );
   RUN_TEST( test_http11_connection_close_header );
   RUN_TEST( test_parse_head_request );
   RUN_TEST( test_parse_put_request );
   RUN_TEST( test_parse_delete_request );
   RUN_TEST( test_http11_header_value_whitespace_trimming );
   RUN_TEST( test_http11_malformed_request_line_missing_version );
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
