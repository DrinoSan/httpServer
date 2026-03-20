#include <string.h>

#include "unity.h"

#include "../HttpResponse.h"

void setUp( void ) {}
void tearDown( void ) {}

// ===== Existing-functionality tests (should pass) =====

//------------------------------------------------------------------------------
void test_serialize_200_with_body( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "Hello";

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // body is "Hello" (5 chars) + "\n" (1 char) = 6
   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 200 OK \r\n" ) );
   TEST_ASSERT_NOT_NULL( strstr( str.data, "Content-Length: 6\r\n" ) );
   TEST_ASSERT_NOT_NULL( strstr( str.data, "Hello\n" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
void test_serialize_404_with_body( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 404;
   strcpy( resp.status_text, "Not Found" );
   resp.body = "<h1>Not Found</h1>";

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 404 Not Found \r\n" ) );
   TEST_ASSERT_NOT_NULL( strstr( str.data, "<h1>Not Found</h1>" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
void test_serialize_null_body( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 204;
   strcpy( resp.status_text, "No Content" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "Content-Length: 0\r\n" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
void test_status_line_format( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // Status line should start with "HTTP/1.1 200 OK \r\n"
   TEST_ASSERT_EQUAL_INT( 0, strncmp( str.data, "HTTP/1.1 200 OK \r\n", 18 ) );

   sand_string_destroy( &str );
}

// ===== TDD tests for HTTP/1.1 compliance (may fail until implemented) =====

//------------------------------------------------------------------------------
// HTTP/1.1: Date header is required in responses (RFC 7231 section 7.1.1.2)
void test_http11_response_includes_date_header( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "hello";

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // Once implemented, Date header should be present in IMF-fixdate format:
   // e.g. "Date: Thu, 20 Mar 2026 12:00:00 GMT\r\n"
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Date: " ) );
   TEST_IGNORE_MESSAGE( "TODO: Add Date header to all responses (RFC 7231 section 7.1.1.2)" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: Server header is recommended (RFC 7231 section 7.4.2)
void test_http11_response_includes_server_header( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "hello";

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // Once implemented:
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Server: " ) );
   TEST_IGNORE_MESSAGE( "TODO: Add Server header to responses" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: Content-Type header should be included with body (RFC 7231 section 3.1.1.5)
void test_http11_response_includes_content_type( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "<h1>Hello</h1>";

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // Currently no Content-Type is serialized
   TEST_IGNORE_MESSAGE( "TODO: Serialize Content-Type header in response" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: HEAD response must have same headers as GET but no body (RFC 7231 section 4.3.2)
void test_http11_head_response_no_body( void )
{
   // HEAD response: same headers as GET, but no body in output
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "<h1>Hello</h1>";   // body exists for Content-Length calculation

   // Once a head-aware serialize is implemented:
   // Sand_string_t str;
   // sand_string_create( &str );
   // http_response_serialize_head( &resp, &str );  // or a flag on resp
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Content-Length: " ) );
   // TEST_ASSERT_NULL( strstr( str.data, "<h1>Hello</h1>" ) );  // no body!
   // sand_string_destroy( &str );
   TEST_IGNORE_MESSAGE( "TODO: HEAD responses should include Content-Length but no body" );
   (void)resp;
}

//------------------------------------------------------------------------------
// HTTP/1.1: Connection: close header in response (RFC 7230 section 6.1)
void test_http11_response_connection_close( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "bye";
   // Set a Connection: close header on the response
   resp.header_count = 1;
   strcpy( resp.headers[ 0 ].name, "Connection" );
   strcpy( resp.headers[ 0 ].value, "close" );

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // Once custom headers are serialized:
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Connection: close\r\n" ) );
   TEST_IGNORE_MESSAGE( "TODO: Serialize Connection: close header in response" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: Support custom response headers
void test_http11_serialize_custom_headers( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "test";
   resp.header_count = 1;
   strcpy( resp.headers[ 0 ].name, "X-Custom" );
   strcpy( resp.headers[ 0 ].value, "my-value" );

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // Currently http_response_serialize doesn't output custom headers
   TEST_IGNORE_MESSAGE( "TODO: Serialize response.headers[] into output" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: Transfer-Encoding: chunked response (RFC 7230 section 3.3.1)
void test_http11_chunked_response( void )
{
   // Chunked response format:
   // Transfer-Encoding: chunked\r\n\r\n
   // <size-hex>\r\n<data>\r\n ... 0\r\n\r\n
   HttpResponse_t resp = { 0 };
   resp.status_code = 200;
   strcpy( resp.status_text, "OK" );
   resp.body = "Hello, World!";

   // Once implemented, a chunked serializer would produce:
   // "HTTP/1.1 200 OK \r\n"
   // "Transfer-Encoding: chunked\r\n\r\n"
   // "d\r\nHello, World!\r\n0\r\n\r\n"
   // Sand_string_t str;
   // sand_string_create( &str );
   // http_response_serialize_chunked( &resp, &str );
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Transfer-Encoding: chunked" ) );
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "d\r\nHello, World!\r\n" ) );
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "0\r\n\r\n" ) );
   // sand_string_destroy( &str );
   TEST_IGNORE_MESSAGE( "TODO: Support chunked transfer-encoding in responses" );
   (void)resp;
}

//------------------------------------------------------------------------------
// HTTP/1.1: 100 Continue interim response (RFC 7231 section 6.2.1)
void test_http11_100_continue_response( void )
{
   // 100 Continue is a simple interim response: "HTTP/1.1 100 Continue\r\n\r\n"
   HttpResponse_t resp = { 0 };
   resp.status_code = 100;
   strcpy( resp.status_text, "Continue" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   // Status line works already, but 100 should have no Content-Length at all
   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 100 Continue" ) );
   // Once implemented, 1xx responses must NOT include Content-Length or body:
   // TEST_ASSERT_NULL( strstr( str.data, "Content-Length" ) );
   TEST_IGNORE_MESSAGE( "TODO: 1xx responses must not include Content-Length header" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 201 Created status (RFC 7231 section 6.3.2)
void test_http11_201_created_response( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 201;
   strcpy( resp.status_text, "Created" );
   resp.body = NULL;
   // Location header pointing to the new resource
   resp.header_count = 1;
   strcpy( resp.headers[ 0 ].name, "Location" );
   strcpy( resp.headers[ 0 ].value, "/resource/42" );

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 201 Created" ) );
   // Once custom headers are serialized:
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Location: /resource/42\r\n" ) );
   TEST_IGNORE_MESSAGE( "TODO: Serialize Location header in 201 response" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 204 No Content — no body, no Content-Length (RFC 7231 section 6.3.5)
void test_http11_204_no_content_no_body( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 204;
   strcpy( resp.status_text, "No Content" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 204 No Content" ) );
   // 204 must NOT include a message body or Content-Length
   // Currently the serializer adds "Content-Length: 0" for NULL body
   // Once implemented:
   // TEST_ASSERT_NULL( strstr( str.data, "Content-Length" ) );
   TEST_IGNORE_MESSAGE( "TODO: 204 response should not include Content-Length" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 301 Moved Permanently with Location header (RFC 7231 section 6.4.2)
void test_http11_301_redirect( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 301;
   strcpy( resp.status_text, "Moved Permanently" );
   resp.body = NULL;
   resp.header_count = 1;
   strcpy( resp.headers[ 0 ].name, "Location" );
   strcpy( resp.headers[ 0 ].value, "https://example.com/new-path" );

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 301 Moved Permanently" ) );
   // Once custom headers are serialized:
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Location: https://example.com/new-path\r\n" ) );
   TEST_IGNORE_MESSAGE( "TODO: Serialize Location header in 301 redirect" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 302 Found redirect (RFC 7231 section 6.4.3)
void test_http11_302_redirect( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 302;
   strcpy( resp.status_text, "Found" );
   resp.body = NULL;
   resp.header_count = 1;
   strcpy( resp.headers[ 0 ].name, "Location" );
   strcpy( resp.headers[ 0 ].value, "/login" );

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 302 Found" ) );
   // Once custom headers are serialized:
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Location: /login\r\n" ) );
   TEST_IGNORE_MESSAGE( "TODO: Serialize Location header in 302 redirect" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 400 Bad Request (RFC 7231 section 6.5.1)
void test_http11_400_bad_request( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 400;
   strcpy( resp.status_text, "Bad Request" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 400 Bad Request \r\n" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 405 Method Not Allowed (RFC 7231 section 6.5.5)
void test_http11_405_method_not_allowed_response( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 405;
   strcpy( resp.status_text, "Method Not Allowed" );
   resp.body = NULL;
   resp.header_count = 1;
   strcpy( resp.headers[ 0 ].name, "Allow" );
   strcpy( resp.headers[ 0 ].value, "GET, HEAD" );

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 405 Method Not Allowed" ) );
   // Once custom headers are serialized:
   // TEST_ASSERT_NOT_NULL( strstr( str.data, "Allow: GET, HEAD\r\n" ) );
   TEST_IGNORE_MESSAGE( "TODO: Serialize Allow header in 405 response" );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 411 Length Required (RFC 7231 section 6.5.10)
void test_http11_411_length_required_response( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 411;
   strcpy( resp.status_text, "Length Required" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 411 Length Required" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 413 Payload Too Large (RFC 7231 section 6.5.11)
void test_http11_413_payload_too_large_response( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 413;
   strcpy( resp.status_text, "Payload Too Large" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 413 Payload Too Large" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 414 URI Too Long (RFC 7231 section 6.5.12)
void test_http11_414_uri_too_long_response( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 414;
   strcpy( resp.status_text, "URI Too Long" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 414 URI Too Long" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 500 Internal Server Error (RFC 7231 section 6.6.1)
void test_http11_500_internal_server_error( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 500;
   strcpy( resp.status_text, "Internal Server Error" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 500 Internal Server Error \r\n" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 501 Not Implemented (RFC 7231 section 6.6.2)
void test_http11_501_not_implemented_response( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 501;
   strcpy( resp.status_text, "Not Implemented" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 501 Not Implemented" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
// HTTP/1.1: 505 HTTP Version Not Supported (RFC 7231 section 6.6.6)
void test_http11_505_version_not_supported_response( void )
{
   HttpResponse_t resp = { 0 };
   resp.status_code = 505;
   strcpy( resp.status_text, "HTTP Version Not Supported" );
   resp.body = NULL;

   Sand_string_t str;
   sand_string_create( &str );
   http_response_serialize( &resp, &str );

   TEST_ASSERT_NOT_NULL( strstr( str.data, "HTTP/1.1 505 HTTP Version Not Supported" ) );

   sand_string_destroy( &str );
}

//------------------------------------------------------------------------------
int main( void )
{
   UNITY_BEGIN();

   // Existing functionality
   RUN_TEST( test_serialize_200_with_body );
   RUN_TEST( test_serialize_404_with_body );
   RUN_TEST( test_serialize_null_body );
   RUN_TEST( test_status_line_format );

   // HTTP/1.1 compliance (TDD)
   RUN_TEST( test_http11_response_includes_date_header );
   RUN_TEST( test_http11_response_includes_server_header );
   RUN_TEST( test_http11_response_includes_content_type );
   RUN_TEST( test_http11_head_response_no_body );
   RUN_TEST( test_http11_response_connection_close );
   RUN_TEST( test_http11_serialize_custom_headers );
   RUN_TEST( test_http11_chunked_response );
   RUN_TEST( test_http11_100_continue_response );
   RUN_TEST( test_http11_201_created_response );
   RUN_TEST( test_http11_204_no_content_no_body );
   RUN_TEST( test_http11_301_redirect );
   RUN_TEST( test_http11_302_redirect );
   RUN_TEST( test_http11_400_bad_request );
   RUN_TEST( test_http11_405_method_not_allowed_response );
   RUN_TEST( test_http11_411_length_required_response );
   RUN_TEST( test_http11_413_payload_too_large_response );
   RUN_TEST( test_http11_414_uri_too_long_response );
   RUN_TEST( test_http11_500_internal_server_error );
   RUN_TEST( test_http11_501_not_implemented_response );
   RUN_TEST( test_http11_505_version_not_supported_response );

   return UNITY_END();
}
