#include <string.h>

#include "unity.h"

#include "../Connection.h"

void setUp( void ) {}
void tearDown( void ) {}

// ===== Existing-functionality tests (should pass) =====

//------------------------------------------------------------------------------
void test_find_existing_header( void )
{
   HttpRequest_t req = { 0 };
   req.header_count = 2;
   strcpy( req.headers[ 0 ].name, "host" );
   strcpy( req.headers[ 0 ].value, "localhost" );
   strcpy( req.headers[ 1 ].name, "content-type" );
   strcpy( req.headers[ 1 ].value, "text/html" );

   const char* val = http_request_find_header( &req, "host" );
   TEST_ASSERT_EQUAL_STRING( "localhost", val );
}

//------------------------------------------------------------------------------
void test_find_nonexistent_header( void )
{
   HttpRequest_t req = { 0 };
   req.header_count = 1;
   strcpy( req.headers[ 0 ].name, "host" );
   strcpy( req.headers[ 0 ].value, "localhost" );

   const char* val = http_request_find_header( &req, "accept" );
   TEST_ASSERT_NULL( val );
}

//------------------------------------------------------------------------------
void test_empty_request_returns_null( void )
{
   HttpRequest_t req = { 0 };
   req.header_count = 0;

   const char* val = http_request_find_header( &req, "host" );
   TEST_ASSERT_NULL( val );
}

//------------------------------------------------------------------------------
void test_find_last_header( void )
{
   HttpRequest_t req = { 0 };
   req.header_count = 3;
   strcpy( req.headers[ 0 ].name, "accept" );
   strcpy( req.headers[ 0 ].value, "text/html" );
   strcpy( req.headers[ 1 ].name, "host" );
   strcpy( req.headers[ 1 ].value, "example.com" );
   strcpy( req.headers[ 2 ].name, "connection" );
   strcpy( req.headers[ 2 ].value, "keep-alive" );

   const char* val = http_request_find_header( &req, "connection" );
   TEST_ASSERT_EQUAL_STRING( "keep-alive", val );
}

// ===== TDD tests for HTTP/1.1 compliance (may fail until implemented) =====

//------------------------------------------------------------------------------
// HTTP/1.1: case-insensitive header name lookup (RFC 7230 section 3.2)
// Header names are case-insensitive, so "Host" and "host" should match
void test_http11_case_insensitive_header_lookup( void )
{
   HttpRequest_t req = { 0 };
   req.header_count = 1;
   strcpy( req.headers[ 0 ].name, "content-type" );
   strcpy( req.headers[ 0 ].value, "text/html" );

   // Parser stores names lowercase, so lookup with lowercase works
   const char* val = http_request_find_header( &req, "content-type" );
   TEST_ASSERT_EQUAL_STRING( "text/html", val );

   // But lookup with mixed case currently fails because strcmp is case-sensitive
   val = http_request_find_header( &req, "Content-Type" );
   // TODO: http_request_find_header should use case-insensitive comparison
   TEST_IGNORE_MESSAGE( "TODO: Make header lookup case-insensitive (RFC 7230 section 3.2)" );
   (void)val;
}

//------------------------------------------------------------------------------
// HTTP/1.1: support duplicate headers (RFC 7230 section 3.2.2)
// Multiple headers with same name should be combined with comma
void test_http11_duplicate_header_names( void )
{
   // Two headers with the same name
   HttpRequest_t req = { 0 };
   req.header_count = 3;
   strcpy( req.headers[ 0 ].name, "host" );
   strcpy( req.headers[ 0 ].value, "localhost" );
   strcpy( req.headers[ 1 ].name, "accept" );
   strcpy( req.headers[ 1 ].value, "text/html" );
   strcpy( req.headers[ 2 ].name, "accept" );
   strcpy( req.headers[ 2 ].value, "application/json" );

   // Once implemented, looking up "accept" should return combined value:
   // const char* val = http_request_find_header( &req, "accept" );
   // TEST_ASSERT_EQUAL_STRING( "text/html, application/json", val );
   TEST_IGNORE_MESSAGE( "TODO: Combine duplicate header values with comma separator" );
   (void)req;
}

//------------------------------------------------------------------------------
// HTTP/1.1: Content-Length must be a valid non-negative integer
void test_http11_content_length_parsed_to_int( void )
{
   HttpRequest_t req = { 0 };
   req.header_count = 1;
   strcpy( req.headers[ 0 ].name, "content-length" );
   strcpy( req.headers[ 0 ].value, "42" );

   const char* val = http_request_find_header( &req, "content-length" );
   TEST_ASSERT_EQUAL_STRING( "42", val );

   // TODO: request.content_length should be automatically populated from header
   TEST_IGNORE_MESSAGE( "TODO: Auto-populate content_length field from Content-Length header" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: Expect: 100-continue (RFC 7231 section 5.1.1)
// Client may send Expect header, server should respond with 100 Continue
void test_http11_expect_100_continue( void )
{
   HttpRequest_t req = { 0 };
   req.header_count = 3;
   strcpy( req.headers[ 0 ].name, "host" );
   strcpy( req.headers[ 0 ].value, "localhost" );
   strcpy( req.headers[ 1 ].name, "content-length" );
   strcpy( req.headers[ 1 ].value, "1024" );
   strcpy( req.headers[ 2 ].name, "expect" );
   strcpy( req.headers[ 2 ].value, "100-continue" );

   const char* val = http_request_find_header( &req, "expect" );
   TEST_ASSERT_EQUAL_STRING( "100-continue", val );

   // Once implemented, server should send "HTTP/1.1 100 Continue\r\n\r\n"
   // before reading the body
   TEST_IGNORE_MESSAGE( "TODO: Server should send 100 Continue before reading body when Expect header present" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: request body reading with Content-Length (RFC 7230 section 3.3)
// After headers are parsed, body should be accessible based on Content-Length
void test_http11_read_body_with_content_length( void )
{
   // Simulate a connection that received a full POST with body
   Connection_t con = { 0 };
   const char* raw = "POST /data HTTP/1.1\r\n"
                     "Host: localhost\r\n"
                     "Content-Length: 13\r\n"
                     "\r\n"
                     "Hello, World!";

   int len = strlen( raw );
   memcpy( con.buffer, raw, len );
   con.bytes_read = len;
   con.header_len = strstr( raw, "\r\n\r\n" ) - raw + 4;
   con.request.content_length = 13;

   // The body pointer should point into buffer after headers
   con.request.body = con.buffer + con.header_len;

   TEST_ASSERT_EQUAL_STRING( "Hello, World!", con.request.body );
   TEST_ASSERT_EQUAL( 13, con.request.content_length );

   // Once body reading is formalized into a function:
   // TEST_ASSERT_EQUAL( 13, con.bytes_read - con.header_len );
   TEST_IGNORE_MESSAGE( "TODO: Formalize body reading into a testable function" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: chunked request body decoding (RFC 7230 section 4.1)
void test_http11_read_chunked_body( void )
{
   // Chunked encoding format:
   // <chunk-size in hex>\r\n<chunk-data>\r\n ... 0\r\n\r\n
   Connection_t con = { 0 };
   const char* raw = "POST /upload HTTP/1.1\r\n"
                     "Host: localhost\r\n"
                     "Transfer-Encoding: chunked\r\n"
                     "\r\n"
                     "5\r\n"
                     "Hello\r\n"
                     "7\r\n"
                     ", World\r\n"
                     "0\r\n"
                     "\r\n";

   int len = strlen( raw );
   memcpy( con.buffer, raw, len );
   con.bytes_read = len;

   // Once chunked decoding is implemented:
   // The decoded body should be "Hello, World" (12 bytes)
   // int decoded_len = http_decode_chunked_body( con.buffer + header_len, ... );
   // TEST_ASSERT_EQUAL( 12, decoded_len );
   // TEST_ASSERT_EQUAL_STRING( "Hello, World", decoded_body );
   TEST_IGNORE_MESSAGE( "TODO: Implement chunked transfer-encoding decoder" );
}

// ===== Keep-alive tests (RFC 7230 section 6.3) =====
//
// How to make keep-alive unit-testable:
//
// Extract a pure function from the server loop, e.g.:
//
//   bool connection_should_keep_alive( const Connection_t* con );
//
// Logic:
//   - HTTP/1.1 + no "Connection: close" header  -> true  (keep-alive is default)
//   - HTTP/1.1 + "Connection: close"             -> false
//   - HTTP/1.0 + no "Connection: keep-alive"     -> false (close is default)
//   - HTTP/1.0 + "Connection: keep-alive"        -> true
//
// This function only reads con->request (version + headers), so it needs
// no sockets and is fully testable here.
//
// For the connection reset after keep-alive, extract:
//
//   void connection_reset_for_next_request( Connection_t* con );
//
// Which resets state/bytes_read/header_len/request/response/buffer
// but keeps fd open. Also fully testable.
//
// Integration tests (real sockets) would then verify:
//   - Two requests on one TCP connection both get responses
//   - Server closes after Connection: close
//   - Idle timeout fires on keep-alive connections

//------------------------------------------------------------------------------
// HTTP/1.1: persistent connections are default (RFC 7230 section 6.3)
// connection_should_keep_alive() should return true for HTTP/1.1 with no Connection header
void test_keepalive_http11_default_is_persistent( void )
{
   // Setup: HTTP/1.1 request with Host but no Connection header
   Connection_t con = { 0 };
   strcpy( con.request.version, "HTTP/1.1" );
   con.request.header_count = 1;
   strcpy( con.request.headers[ 0 ].name, "host" );
   strcpy( con.request.headers[ 0 ].value, "localhost" );

   // Once connection_should_keep_alive() is implemented:
   // TEST_ASSERT_TRUE( connection_should_keep_alive( &con ) );
   TEST_IGNORE_MESSAGE( "TODO: Implement connection_should_keep_alive() — HTTP/1.1 defaults to keep-alive" );
   (void)con;
}

//------------------------------------------------------------------------------
// HTTP/1.1: Connection: close overrides keep-alive default
void test_keepalive_http11_connection_close( void )
{
   Connection_t con = { 0 };
   strcpy( con.request.version, "HTTP/1.1" );
   con.request.header_count = 2;
   strcpy( con.request.headers[ 0 ].name, "host" );
   strcpy( con.request.headers[ 0 ].value, "localhost" );
   strcpy( con.request.headers[ 1 ].name, "connection" );
   strcpy( con.request.headers[ 1 ].value, "close" );

   // Once connection_should_keep_alive() is implemented:
   // TEST_ASSERT_FALSE( connection_should_keep_alive( &con ) );
   TEST_IGNORE_MESSAGE( "TODO: Implement connection_should_keep_alive() — Connection: close disables keep-alive" );
   (void)con;
}

//------------------------------------------------------------------------------
// HTTP/1.1: explicit keep-alive header (redundant but valid)
void test_keepalive_http11_explicit_keepalive( void )
{
   Connection_t con = { 0 };
   strcpy( con.request.version, "HTTP/1.1" );
   con.request.header_count = 2;
   strcpy( con.request.headers[ 0 ].name, "host" );
   strcpy( con.request.headers[ 0 ].value, "localhost" );
   strcpy( con.request.headers[ 1 ].name, "connection" );
   strcpy( con.request.headers[ 1 ].value, "keep-alive" );

   // TEST_ASSERT_TRUE( connection_should_keep_alive( &con ) );
   TEST_IGNORE_MESSAGE( "TODO: Implement connection_should_keep_alive() — explicit keep-alive in HTTP/1.1" );
   (void)con;
}

//------------------------------------------------------------------------------
// HTTP/1.0: connections close by default (RFC 7230 section 6.3)
void test_keepalive_http10_default_is_close( void )
{
   Connection_t con = { 0 };
   strcpy( con.request.version, "HTTP/1.0" );
   con.request.header_count = 1;
   strcpy( con.request.headers[ 0 ].name, "host" );
   strcpy( con.request.headers[ 0 ].value, "localhost" );

   // TEST_ASSERT_FALSE( connection_should_keep_alive( &con ) );
   TEST_IGNORE_MESSAGE( "TODO: Implement connection_should_keep_alive() — HTTP/1.0 defaults to close" );
   (void)con;
}

//------------------------------------------------------------------------------
// HTTP/1.0: Connection: keep-alive enables persistence
void test_keepalive_http10_explicit_keepalive( void )
{
   Connection_t con = { 0 };
   strcpy( con.request.version, "HTTP/1.0" );
   con.request.header_count = 2;
   strcpy( con.request.headers[ 0 ].name, "host" );
   strcpy( con.request.headers[ 0 ].value, "localhost" );
   strcpy( con.request.headers[ 1 ].name, "connection" );
   strcpy( con.request.headers[ 1 ].value, "keep-alive" );

   // TEST_ASSERT_TRUE( connection_should_keep_alive( &con ) );
   TEST_IGNORE_MESSAGE( "TODO: Implement connection_should_keep_alive() — HTTP/1.0 + keep-alive header" );
   (void)con;
}

//------------------------------------------------------------------------------
// Keep-alive: connection reset between requests
// After responding, connection state should be reset for the next request
// but fd must remain open
void test_keepalive_connection_reset( void )
{
   Connection_t con = { 0 };
   con.fd         = 42;
   con.bytes_read = 512;
   con.header_len = 128;
   con.state      = CONN_SENDING_RESPONSE;
   strcpy( con.request.method, "GET" );
   strcpy( con.request.path, "/old" );
   con.request.header_count = 1;
   strcpy( con.request.headers[ 0 ].name, "host" );
   strcpy( con.request.headers[ 0 ].value, "localhost" );

   // Once connection_reset_for_next_request() is implemented:
   // connection_reset_for_next_request( &con );
   // TEST_ASSERT_EQUAL( 42, con.fd );                     // fd stays open
   // TEST_ASSERT_EQUAL( CONN_READING_HEADERS, con.state );
   // TEST_ASSERT_EQUAL( 0, con.bytes_read );
   // TEST_ASSERT_EQUAL( 0, con.header_len );
   // TEST_ASSERT_EQUAL( 0, con.request.header_count );
   // TEST_ASSERT_EQUAL_STRING( "", con.request.method );
   TEST_IGNORE_MESSAGE( "TODO: Implement connection_reset_for_next_request() for keep-alive reuse" );
   (void)con;
}

//------------------------------------------------------------------------------
// Keep-alive: response should include Connection header
// HTTP/1.1 response to Connection: close should echo it back
void test_keepalive_response_echoes_connection_close( void )
{
   // When the server decides to close, the response should include
   // Connection: close so the client knows
   Connection_t con = { 0 };
   con.response.status_code = 200;
   strcpy( con.response.status_text, "OK" );
   con.response.body = "bye";

   // Once implemented, the serializer should include Connection: close:
   // Sand_string_t str;
   // sand_string_create( &str );
   // http_response_serialize( &con.response, &str );
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Connection: close\r\n" ) );
   // sand_string_destroy( &str );
   TEST_IGNORE_MESSAGE( "TODO: Response should include Connection: close when closing" );
   (void)con;
}

//------------------------------------------------------------------------------
int main( void )
{
   UNITY_BEGIN();

   // Existing functionality
   RUN_TEST( test_find_existing_header );
   RUN_TEST( test_find_nonexistent_header );
   RUN_TEST( test_empty_request_returns_null );
   RUN_TEST( test_find_last_header );

   // HTTP/1.1 compliance (TDD)
   RUN_TEST( test_http11_case_insensitive_header_lookup );
   RUN_TEST( test_http11_duplicate_header_names );
   RUN_TEST( test_http11_content_length_parsed_to_int );
   RUN_TEST( test_http11_expect_100_continue );
   RUN_TEST( test_http11_read_body_with_content_length );
   RUN_TEST( test_http11_read_chunked_body );
   // Keep-alive (RFC 7230 section 6.3)
   RUN_TEST( test_keepalive_http11_default_is_persistent );
   RUN_TEST( test_keepalive_http11_connection_close );
   RUN_TEST( test_keepalive_http11_explicit_keepalive );
   RUN_TEST( test_keepalive_http10_default_is_close );
   RUN_TEST( test_keepalive_http10_explicit_keepalive );
   RUN_TEST( test_keepalive_connection_reset );
   RUN_TEST( test_keepalive_response_echoes_connection_close );

   return UNITY_END();
}
