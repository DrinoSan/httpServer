#include <string.h>

#include "unity.h"

#include "../Router.h"

// Dummy handlers for testing
static void handler_home( Connection_t* con )    { (void)con; }
static void handler_about( Connection_t* con )   { (void)con; }
static void handler_api( Connection_t* con )     { (void)con; }

static Router_t router;

void setUp( void )
{
   memset( &router, 0, sizeof( router ) );
}

void tearDown( void ) {}

// ===== Existing-functionality tests (should pass) =====

//------------------------------------------------------------------------------
void test_add_route_then_find( void )
{
   router_add_route( &router, "GET", "/", handler_home );
   RouteHandler_t found = router_find_route( &router, "GET", "/" );
   TEST_ASSERT_EQUAL_PTR( handler_home, found );
}

//------------------------------------------------------------------------------
void test_find_nonexistent_route_returns_404_handler( void )
{
   router_add_route( &router, "GET", "/", handler_home );
   RouteHandler_t found = router_find_route( &router, "GET", "/nope" );

   // Should return a handler (the 404 handler), not NULL
   TEST_ASSERT_NOT_NULL( found );
   TEST_ASSERT_NOT_EQUAL( handler_home, found );
}

//------------------------------------------------------------------------------
void test_add_multiple_routes_find_each( void )
{
   router_add_route( &router, "GET", "/", handler_home );
   router_add_route( &router, "GET", "/about", handler_about );
   router_add_route( &router, "POST", "/api", handler_api );

   TEST_ASSERT_EQUAL_PTR( handler_home, router_find_route( &router, "GET", "/" ) );
   TEST_ASSERT_EQUAL_PTR( handler_about, router_find_route( &router, "GET", "/about" ) );
   TEST_ASSERT_EQUAL_PTR( handler_api, router_find_route( &router, "POST", "/api" ) );
}

//------------------------------------------------------------------------------
void test_method_too_long_not_added( void )
{
   // method buffer is 8 chars, so "LONGMETHOD" (10 chars) should be rejected
   router_add_route( &router, "LONGMETHOD", "/", handler_home );
   TEST_ASSERT_EQUAL( 0, router.count_routes );
}

//------------------------------------------------------------------------------
void test_path_too_long_not_added( void )
{
   // path buffer is 256 chars
   char long_path[ 300 ];
   memset( long_path, 'a', 299 );
   long_path[ 0 ]   = '/';
   long_path[ 299 ] = '\0';

   router_add_route( &router, "GET", long_path, handler_home );
   TEST_ASSERT_EQUAL( 0, router.count_routes );
}

//------------------------------------------------------------------------------
void test_exceed_max_routes( void )
{
   char path[ 16 ];
   for ( int32_t i = 0; i < MAX_ROUTES; i++ )
   {
      snprintf( path, sizeof( path ), "/%d", i );
      router_add_route( &router, "GET", path, handler_home );
   }
   TEST_ASSERT_EQUAL( MAX_ROUTES, router.count_routes );

   // One more should not be added
   router_add_route( &router, "GET", "/overflow", handler_home );
   TEST_ASSERT_EQUAL( MAX_ROUTES, router.count_routes );
}

//------------------------------------------------------------------------------
void test_exact_match_only( void )
{
   router_add_route( &router, "GET", "/home", handler_home );

   // Similar but different paths should not match
   RouteHandler_t found = router_find_route( &router, "GET", "/homes" );
   TEST_ASSERT_NOT_EQUAL( handler_home, found );

   found = router_find_route( &router, "GET", "/hom" );
   TEST_ASSERT_NOT_EQUAL( handler_home, found );

   found = router_find_route( &router, "POST", "/home" );
   TEST_ASSERT_NOT_EQUAL( handler_home, found );
}

// ===== TDD tests for HTTP/1.1 compliance (may fail until implemented) =====

//------------------------------------------------------------------------------
// HTTP/1.1: must support HEAD method routes (RFC 7231 section 4.3.2)
// HEAD should behave like GET but return no body
void test_http11_head_route_matching( void )
{
   router_add_route( &router, "HEAD", "/", handler_home );
   RouteHandler_t found = router_find_route( &router, "HEAD", "/" );
   TEST_ASSERT_EQUAL_PTR( handler_home, found );
}

//------------------------------------------------------------------------------
// HTTP/1.1: OPTIONS method support (RFC 7231 section 4.3.7)
// OPTIONS * HTTP/1.1 is a valid request
void test_http11_options_route( void )
{
   router_add_route( &router, "OPTIONS", "/", handler_home );

   // OPTIONS has 7 chars, fits in method[8]
   RouteHandler_t found = router_find_route( &router, "OPTIONS", "/" );
   TEST_ASSERT_EQUAL_PTR( handler_home, found );
}

//------------------------------------------------------------------------------
// HTTP/1.1: method matching should be case-sensitive (RFC 7230 section 3.1.1)
void test_http11_method_case_sensitive( void )
{
   router_add_route( &router, "GET", "/", handler_home );

   RouteHandler_t found = router_find_route( &router, "get", "/" );
   TEST_ASSERT_NOT_EQUAL( handler_home, found );
}

//------------------------------------------------------------------------------
// HTTP/1.1: server must support at least GET and HEAD (RFC 7231 section 4.1)
// This is more of an integration concern, but we can test that 404 handler
// properly sets the response status
void test_http11_404_handler_sets_response( void )
{
   RouteHandler_t handler = router_find_route( &router, "GET", "/nonexistent" );
   TEST_ASSERT_NOT_NULL( handler );

   // Call the 404 handler and check it sets status code
   Connection_t con = { 0 };
   handler( &con );
   TEST_ASSERT_EQUAL( 404, con.response.status_code );
   TEST_ASSERT_EQUAL_STRING( "Not Found", con.response.status_text );
}

//------------------------------------------------------------------------------
// HTTP/1.1: should support query strings in paths (RFC 7230 section 2.7)
// Currently the router does exact match, so /path?query won't match /path
void test_http11_path_with_query_string( void )
{
   router_add_route( &router, "GET", "/search", handler_home );

   // With query string - currently won't match because of exact match
   RouteHandler_t found = router_find_route( &router, "GET", "/search?q=test" );

   // TODO: Implement query string stripping for route matching
   TEST_IGNORE_MESSAGE( "TODO: Strip query string from path before route matching" );
   (void)found;
}

//------------------------------------------------------------------------------
// HTTP/1.1: should respond 405 Method Not Allowed when path exists but method doesn't
// (RFC 7231 section 6.5.5)
void test_http11_method_not_allowed( void )
{
   // Register GET /resource but request POST /resource
   router_add_route( &router, "GET", "/resource", handler_home );

   RouteHandler_t found = router_find_route( &router, "POST", "/resource" );

   // Currently returns 404 handler — should return a 405 handler instead
   // Once implemented:
   // Connection_t con = { 0 };
   // found( &con );
   // TEST_ASSERT_EQUAL( 405, con.response.status_code );
   TEST_IGNORE_MESSAGE( "TODO: Return 405 when path matches but method doesn't" );
   (void)found;
}

//------------------------------------------------------------------------------
// HTTP/1.1: TRACE method (RFC 7231 section 4.3.8)
// Server should either support TRACE or explicitly reject it
void test_http11_trace_method( void )
{
   // TRACE is 5 chars, fits in method[8]
   // Server should either handle it or return 405/501
   RouteHandler_t found = router_find_route( &router, "TRACE", "/" );

   // Once implemented — either a dedicated TRACE handler or explicit rejection:
   // Connection_t con = { 0 };
   // found( &con );
   // TEST_ASSERT_TRUE( con.response.status_code == 200 ||
   //                   con.response.status_code == 405 ||
   //                   con.response.status_code == 501 );
   TEST_IGNORE_MESSAGE( "TODO: Support or explicitly reject TRACE method" );
   (void)found;
}

//------------------------------------------------------------------------------
// HTTP/1.1: CONNECT method (RFC 7231 section 4.3.6)
// Server should handle or reject CONNECT
void test_http11_connect_method( void )
{
   // CONNECT is 7 chars, fits in method[8]
   RouteHandler_t found = router_find_route( &router, "CONNECT", "localhost:443" );

   // Once implemented:
   // Connection_t con = { 0 };
   // found( &con );
   // TEST_ASSERT_TRUE( con.response.status_code == 405 ||
   //                   con.response.status_code == 501 );
   TEST_IGNORE_MESSAGE( "TODO: Handle or reject CONNECT method" );
   (void)found;
}

//------------------------------------------------------------------------------
// HTTP/1.1: 501 Not Implemented for unrecognized methods (RFC 7231 section 6.6.2)
void test_http11_unknown_method_501( void )
{
   // FOOBAR is not a standard HTTP method
   RouteHandler_t found = router_find_route( &router, "FOOBAR", "/" );

   // Currently returns 404 handler — should return 501 handler
   // Once implemented:
   // Connection_t con = { 0 };
   // found( &con );
   // TEST_ASSERT_EQUAL( 501, con.response.status_code );
   TEST_IGNORE_MESSAGE( "TODO: Return 501 Not Implemented for unrecognized methods (e.g. PATCH, FOOBAR)" );
   (void)found;
}

//------------------------------------------------------------------------------
// HTTP/1.1: 405 response MUST include Allow header listing valid methods (RFC 7231 section 6.5.5)
void test_http11_405_includes_allow_header( void )
{
   // Register GET and HEAD for /resource
   router_add_route( &router, "GET", "/resource", handler_home );
   router_add_route( &router, "HEAD", "/resource", handler_about );

   // Request with POST — path exists but method doesn't
   RouteHandler_t found = router_find_route( &router, "POST", "/resource" );

   // Once implemented, the 405 handler should set an Allow header:
   // Connection_t con = { 0 };
   // found( &con );
   // TEST_ASSERT_EQUAL( 405, con.response.status_code );
   // const char* allow = /* find Allow header in response */;
   // TEST_ASSERT_NOT_NULL( strstr( allow, "GET" ) );
   // TEST_ASSERT_NOT_NULL( strstr( allow, "HEAD" ) );
   TEST_IGNORE_MESSAGE( "TODO: 405 response must include Allow header (e.g. Allow: GET, HEAD)" );
   (void)found;
}

//------------------------------------------------------------------------------
// HTTP/1.1: OPTIONS * for server-wide capabilities (RFC 7231 section 4.3.7)
void test_http11_options_asterisk( void )
{
   // OPTIONS * is a valid server-wide request
   router_add_route( &router, "GET", "/", handler_home );

   RouteHandler_t found = router_find_route( &router, "OPTIONS", "*" );

   // Once implemented, should return a handler that lists all supported methods:
   // Connection_t con = { 0 };
   // found( &con );
   // TEST_ASSERT_EQUAL( 200, con.response.status_code );
   // Should include Allow header with all methods the server supports
   TEST_IGNORE_MESSAGE( "TODO: Handle OPTIONS * request for server-wide capabilities" );
   (void)found;
}

//------------------------------------------------------------------------------
int main( void )
{
   UNITY_BEGIN();

   // Existing functionality
   RUN_TEST( test_add_route_then_find );
   RUN_TEST( test_find_nonexistent_route_returns_404_handler );
   RUN_TEST( test_add_multiple_routes_find_each );
   RUN_TEST( test_method_too_long_not_added );
   RUN_TEST( test_path_too_long_not_added );
   RUN_TEST( test_exceed_max_routes );
   RUN_TEST( test_exact_match_only );

   // HTTP/1.1 compliance (TDD)
   RUN_TEST( test_http11_head_route_matching );
   RUN_TEST( test_http11_options_route );
   RUN_TEST( test_http11_method_case_sensitive );
   RUN_TEST( test_http11_404_handler_sets_response );
   RUN_TEST( test_http11_path_with_query_string );
   RUN_TEST( test_http11_method_not_allowed );
   RUN_TEST( test_http11_trace_method );
   RUN_TEST( test_http11_connect_method );
   RUN_TEST( test_http11_unknown_method_501 );
   RUN_TEST( test_http11_405_includes_allow_header );
   RUN_TEST( test_http11_options_asterisk );

   return UNITY_END();
}
