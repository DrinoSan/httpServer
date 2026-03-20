#include <string.h>

#include "unity.h"

#include "../HttpRequest.h"

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
   TEST_IGNORE_MESSAGE( "TODO: Combine duplicate header values with comma separator" );
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
   TEST_IGNORE_MESSAGE( "TODO: Handle Expect: 100-continue header" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: request body reading with Content-Length (RFC 7230 section 3.3)
// After headers are parsed, body should be accessible based on Content-Length
void test_http11_read_body_with_content_length( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Read and store request body based on Content-Length" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: chunked request body decoding (RFC 7230 section 4.1)
void test_http11_read_chunked_body( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Decode chunked request body" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: persistent connections are default (RFC 7230 section 6.3)
// Request should track whether connection should stay open
void test_http11_persistent_connection_default( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Track keep-alive state — default persistent in HTTP/1.1" );
}

//------------------------------------------------------------------------------
// HTTP/1.1: Connection: close overrides keep-alive default
void test_http11_connection_close_overrides_keepalive( void )
{
   TEST_IGNORE_MESSAGE( "TODO: Connection: close should mark connection for closing" );
}

//------------------------------------------------------------------------------
// HTTP/1.0: connections close by default (no keep-alive unless explicitly requested)
void test_http10_connection_closes_by_default( void )
{
   TEST_IGNORE_MESSAGE( "TODO: HTTP/1.0 connections should default to close" );
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
   RUN_TEST( test_http11_persistent_connection_default );
   RUN_TEST( test_http11_connection_close_overrides_keepalive );
   RUN_TEST( test_http10_connection_closes_by_default );

   return UNITY_END();
}
