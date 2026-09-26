#include <string.h>
#include <sys/event.h>
#include <unistd.h>

#include "unity.h"

#include "../Connection.h"
#include "../HttpParser.h"
#include "../Server.h"

// ============================================================================
// Keep-alive / connection-persistence tests (RFC 7230 section 6.3)
//
// These drive the REAL code path, not a re-implementation:
//
//   raw request bytes -> http_parser_parse_request()
//                     -> server_request_keep_alive_check()
//                     -> con.is_keep_alive
//
// Going through the parser matters: server_request_keep_alive_check() reads
// request.version_int (not the version string) and looks the Connection
// header up by its lowercased name, so a hand-built HttpRequest_t would test
// something the server never actually does.
//
// Socket-level behaviour (idle timeouts, slowloris, real connection reuse)
// cannot be covered here -- it needs a running server. Those are listed as
// TEST_IGNORE at the bottom so they stay visible.
// ============================================================================

// kqueue fd shared by the fixtures. connection_reset() issues a kevent()
// against con->kqueueFd, so it has to be a real kqueue or the call fails
// silently and we would not be testing what we think we are.
static int g_kq = -1;

void setUp( void ) {}
void tearDown( void ) {}

//------------------------------------------------------------------------------
// Bring a Connection_t to the state connection_create_heap() would leave it in,
// minus the socket. fd is -1 on purpose: nothing here may close() it.
static void init_con( Connection_t* con )
{
   memset( con, 0, sizeof( *con ) );
   sand_string_create( &con->buf );
   sand_string_create( &con->buf_for_error_405 );
   con->fd            = -1;
   con->kqueueFd      = g_kq;
   con->state         = CONN_READING_HEADERS;
   con->is_keep_alive = false;
}

//------------------------------------------------------------------------------
// Feed raw request bytes through the real parser, exactly like the worker loop
// does: copy into con->buffer, locate the CRLFCRLF, set header_len, parse.
static void parse_into( Connection_t* con, const char* raw )
{
   size_t len = strlen( raw );
   TEST_ASSERT_TRUE_MESSAGE( len < BUFFER_SIZE, "fixture larger than buffer" );

   memset( con->buffer, 0, BUFFER_SIZE );
   memcpy( con->buffer, raw, len );
   con->bytes_read = ( int32_t ) len;

   char* end = strstr( con->buffer, "\r\n\r\n" );
   TEST_ASSERT_NOT_NULL_MESSAGE( end, "fixture must end with CRLFCRLF" );
   con->header_len = ( int32_t ) ( end - con->buffer ) + 4;

   ParseResult_t res =
       http_parser_parse_request( con->buffer, con->header_len, &con->request );
   TEST_ASSERT_EQUAL_MESSAGE( PARSE_OK, res, "fixture failed to parse" );
}

//------------------------------------------------------------------------------
// Full pipeline for a single request: parse, decide, report the flag.
static bool keep_alive_for( const char* raw )
{
   Connection_t con;
   init_con( &con );
   parse_into( &con, raw );

   server_request_keep_alive_check( &con );

   return con.is_keep_alive;
}

// Request fixtures. Second %s slot is the extra header block (may be empty).
#define REQ_11( extra ) "GET /home HTTP/1.1\r\nHost: localhost\r\n" extra "\r\n"
#define REQ_10( extra ) "GET /home HTTP/1.0\r\nHost: localhost\r\n" extra "\r\n"

// ============================================================================
// HTTP/1.1 -- persistent by default (RFC 7230 section 6.3)
// ============================================================================

//------------------------------------------------------------------------------
void test_ka_http11_default_is_persistent( void )
{
   TEST_ASSERT_TRUE( keep_alive_for( REQ_11( "" ) ) );
}

//------------------------------------------------------------------------------
void test_ka_http11_connection_close( void )
{
   TEST_ASSERT_FALSE( keep_alive_for( REQ_11( "Connection: close\r\n" ) ) );
}

//------------------------------------------------------------------------------
void test_ka_http11_explicit_keepalive( void )
{
   TEST_ASSERT_TRUE( keep_alive_for( REQ_11( "Connection: keep-alive\r\n" ) ) );
}

//------------------------------------------------------------------------------
// "close" anywhere in the token list wins over keep-alive.
void test_ka_http11_keepalive_and_close_closes( void )
{
   TEST_ASSERT_FALSE(
       keep_alive_for( REQ_11( "Connection: keep-alive, close\r\n" ) ) );
}

//------------------------------------------------------------------------------
// Connection header sitting behind other headers must still be found.
void test_ka_http11_close_after_other_headers( void )
{
   TEST_ASSERT_FALSE( keep_alive_for( REQ_11(
       "Accept: */*\r\nUser-Agent: test\r\nConnection: close\r\n" ) ) );
}

// ============================================================================
// HTTP/1.0 -- closes by default, opt in via Connection: keep-alive
// ============================================================================

//------------------------------------------------------------------------------
void test_ka_http10_default_is_close( void )
{
   TEST_ASSERT_FALSE( keep_alive_for( REQ_10( "" ) ) );
}

//------------------------------------------------------------------------------
void test_ka_http10_explicit_keepalive( void )
{
   TEST_ASSERT_TRUE( keep_alive_for( REQ_10( "Connection: keep-alive\r\n" ) ) );
}

//------------------------------------------------------------------------------
void test_ka_http10_connection_close( void )
{
   TEST_ASSERT_FALSE( keep_alive_for( REQ_10( "Connection: close\r\n" ) ) );
}

// ============================================================================
// Case-insensitivity.
//
// These are the regression guard for sand_string_view_has_substr_no_case().
// Header VALUES are compared as-is by the server, so if that function ever
// stops folding case, every one of these flips.
// ============================================================================

//------------------------------------------------------------------------------
void test_ka_http11_close_uppercase_value( void )
{
   TEST_ASSERT_FALSE( keep_alive_for( REQ_11( "Connection: CLOSE\r\n" ) ) );
}

//------------------------------------------------------------------------------
void test_ka_http11_close_mixed_case_value( void )
{
   TEST_ASSERT_FALSE( keep_alive_for( REQ_11( "Connection: Close\r\n" ) ) );
}

//------------------------------------------------------------------------------
void test_ka_http10_keepalive_mixed_case_value( void )
{
   TEST_ASSERT_TRUE( keep_alive_for( REQ_10( "Connection: Keep-Alive\r\n" ) ) );
}

//------------------------------------------------------------------------------
void test_ka_http10_keepalive_uppercase_value( void )
{
   TEST_ASSERT_TRUE( keep_alive_for( REQ_10( "Connection: KEEP-ALIVE\r\n" ) ) );
}

//------------------------------------------------------------------------------
// Header NAMES are case-insensitive too (RFC 7230 section 3.2); the parser
// lowercases them, so lookup by "connection" must still hit.
void test_ka_header_name_case_insensitive( void )
{
   TEST_ASSERT_FALSE( keep_alive_for( REQ_11( "CONNECTION: close\r\n" ) ) );
}

// ============================================================================
// Connection reuse -- regression guard for the stale-flag bug.
//
// is_keep_alive used to be computed only on the response path, below an early
// return taken whenever the response had no body. On a reused connection the
// flag then carried over from the PREVIOUS request. These pin the fix: the
// decision is recomputed per request, from that request alone.
// ============================================================================

//------------------------------------------------------------------------------
// Persistent request followed by one asking to close -> must close.
void test_ka_reuse_persistent_then_close( void )
{
   Connection_t con;
   init_con( &con );

   parse_into( &con, REQ_11( "" ) );
   server_request_keep_alive_check( &con );
   TEST_ASSERT_TRUE_MESSAGE( con.is_keep_alive, "first request should persist" );

   connection_reset( &con );

   parse_into( &con, REQ_11( "Connection: close\r\n" ) );
   server_request_keep_alive_check( &con );
   TEST_ASSERT_FALSE_MESSAGE( con.is_keep_alive,
                              "stale keep-alive carried into request 2" );
}

//------------------------------------------------------------------------------
// Version downgrade across one connection: 1.1 then a bare 1.0 -> must close.
void test_ka_reuse_http11_then_http10( void )
{
   Connection_t con;
   init_con( &con );

   parse_into( &con, REQ_11( "" ) );
   server_request_keep_alive_check( &con );
   TEST_ASSERT_TRUE( con.is_keep_alive );

   connection_reset( &con );

   parse_into( &con, REQ_10( "" ) );
   server_request_keep_alive_check( &con );
   TEST_ASSERT_FALSE_MESSAGE( con.is_keep_alive,
                              "HTTP/1.0 inherited persistence from HTTP/1.1" );
}

//------------------------------------------------------------------------------
// And the other direction: a closing request must not poison the next one.
void test_ka_reuse_close_then_persistent( void )
{
   Connection_t con;
   init_con( &con );

   parse_into( &con, REQ_11( "Connection: close\r\n" ) );
   server_request_keep_alive_check( &con );
   TEST_ASSERT_FALSE( con.is_keep_alive );

   connection_reset( &con );

   parse_into( &con, REQ_11( "" ) );
   server_request_keep_alive_check( &con );
   TEST_ASSERT_TRUE_MESSAGE( con.is_keep_alive,
                             "request 2 wrongly inherited close" );
}

//------------------------------------------------------------------------------
// The decision must not depend on how the previous response was shaped, so a
// pre-set flag has to be overwritten either way.
void test_ka_check_overwrites_preexisting_flag( void )
{
   Connection_t con;
   init_con( &con );
   con.is_keep_alive = true;   // as if left over from an earlier request

   parse_into( &con, REQ_10( "" ) );
   server_request_keep_alive_check( &con );
   TEST_ASSERT_FALSE( con.is_keep_alive );
}

// ============================================================================
// connection_reset() -- what must and must not survive a reuse
// ============================================================================

//------------------------------------------------------------------------------
void test_ka_reset_clears_request_state_but_keeps_fd( void )
{
   Connection_t con;
   init_con( &con );
   con.fd = 42;   // never closed by connection_reset()

   parse_into( &con, REQ_11( "Connection: close\r\n" ) );
   server_request_keep_alive_check( &con );
   con.state = CONN_SENDING_RESPONSE;

   connection_reset( &con );

   TEST_ASSERT_EQUAL_MESSAGE( 42, con.fd, "fd must stay open for reuse" );
   TEST_ASSERT_EQUAL( CONN_READING_HEADERS, con.state );
   TEST_ASSERT_EQUAL( 0, con.bytes_read );
   TEST_ASSERT_EQUAL( 0, con.header_len );
   TEST_ASSERT_EQUAL( 0, con.request.header_count );
   TEST_ASSERT_FALSE_MESSAGE( con.is_keep_alive,
                              "reset must clear the keep-alive flag" );
}

//------------------------------------------------------------------------------
// Stale headers must not be findable after a reset, or request 2 would see
// request 1's Connection header.
void test_ka_reset_drops_previous_headers( void )
{
   Connection_t con;
   init_con( &con );

   parse_into( &con, REQ_11( "Connection: close\r\n" ) );
   TEST_ASSERT_NOT_NULL( http_request_find_header( &con.request, "connection" ) );

   connection_reset( &con );

   TEST_ASSERT_NULL_MESSAGE(
       http_request_find_header( &con.request, "connection" ),
       "Connection header survived reset" );
}

// ============================================================================
// Version handling -- groundwork for both 1.0 and 1.1
// ============================================================================

//------------------------------------------------------------------------------
void test_ka_version_int_http11( void )
{
   Connection_t con;
   init_con( &con );
   parse_into( &con, REQ_11( "" ) );

   TEST_ASSERT_EQUAL( 1001, con.request.version_int );
   TEST_ASSERT_EQUAL( 1, con.request.http_major );
   TEST_ASSERT_EQUAL( 1, con.request.http_minor );
}

//------------------------------------------------------------------------------
void test_ka_version_int_http10( void )
{
   Connection_t con;
   init_con( &con );
   parse_into( &con, REQ_10( "" ) );

   TEST_ASSERT_EQUAL( 1000, con.request.version_int );
   TEST_ASSERT_EQUAL( 1, con.request.http_major );
   TEST_ASSERT_EQUAL( 0, con.request.http_minor );
}

//------------------------------------------------------------------------------
// HTTP/1.1 requires Host (RFC 7230 section 5.4).
void test_ka_http11_missing_host_rejected( void )
{
   Connection_t con;
   init_con( &con );

   char raw[] = "GET /home HTTP/1.1\r\nAccept: */*\r\n\r\n";
   memcpy( con.buffer, raw, sizeof( raw ) );
   con.header_len = ( int32_t ) strlen( raw );

   ParseResult_t res =
       http_parser_parse_request( con.buffer, con.header_len, &con.request );
   TEST_ASSERT_EQUAL( PARSE_ERROR_MISSING_HOST, res );
}

//------------------------------------------------------------------------------
// HTTP/1.0 has no Host requirement, so the same request must parse.
void test_ka_http10_missing_host_allowed( void )
{
   Connection_t con;
   init_con( &con );

   char raw[] = "GET /home HTTP/1.0\r\nAccept: */*\r\n\r\n";
   memcpy( con.buffer, raw, sizeof( raw ) );
   con.header_len = ( int32_t ) strlen( raw );

   ParseResult_t res =
       http_parser_parse_request( con.buffer, con.header_len, &con.request );
   TEST_ASSERT_EQUAL( PARSE_OK, res );

   // ...and it still defaults to close.
   server_request_keep_alive_check( &con );
   TEST_ASSERT_FALSE( con.is_keep_alive );
}

//------------------------------------------------------------------------------
// Only 1.0 and 1.1 are served; anything else is refused before keep-alive is
// ever consulted. This is what lets server_request_keep_alive_check() assume
// version_int is one of two values.
void test_ka_unsupported_versions_rejected( void )
{
   const char* fixtures[] = {
       "GET /home HTTP/2.0\r\nHost: localhost\r\n\r\n",
       "GET /home HTTP/1.2\r\nHost: localhost\r\n\r\n",
   };

   for ( size_t i = 0; i < sizeof( fixtures ) / sizeof( fixtures[ 0 ] ); i++ )
   {
      Connection_t con;
      init_con( &con );

      size_t len = strlen( fixtures[ i ] );
      memcpy( con.buffer, fixtures[ i ], len );
      con.header_len = ( int32_t ) len;

      ParseResult_t res =
          http_parser_parse_request( con.buffer, con.header_len, &con.request );
      TEST_ASSERT_EQUAL_MESSAGE( PARSE_ERROR_MALFORMED_REQUEST_LINE, res,
                                 fixtures[ i ] );
   }
}

// ============================================================================
// Known-open issues.
//
// Written out so the expected behaviour is already recorded; each is one
// uncomment away from going green when the feature lands.
// ============================================================================

//------------------------------------------------------------------------------
// The response never carries a Connection header today. An HTTP/1.0 client
// treats a missing "Connection: keep-alive" as "server is closing" and will
// not reuse the socket -- so the 1.0 keep-alive path above is decided
// correctly server-side but has no effect on the wire.
void test_ka_response_echoes_keepalive_for_http10( void )
{
   // Connection_t con;
   // init_con( &con );
   // parse_into( &con, REQ_10( "Connection: keep-alive\r\n" ) );
   // server_request_keep_alive_check( &con );
   // con.response.status_code = 200;
   // con.response.body        = "hi";
   //
   // http_server_set_default_headers_for_response( &con );
   // http_response_serialize( &con.response, &con.buf );
   // TEST_ASSERT_NOT_NULL( strstr( con.buf.data, "Connection: keep-alive" ) );
   TEST_IGNORE_MESSAGE(
       "TODO: echo 'Connection: keep-alive' on HTTP/1.0 responses when the "
       "connection is being kept open" );
}

//------------------------------------------------------------------------------
// Same gap in the other direction: when the server has decided to close, it
// should say so, or a client may keep pipelining into a socket about to die.
void test_ka_response_sends_connection_close_when_closing( void )
{
   // Connection_t con;
   // init_con( &con );
   // parse_into( &con, REQ_11( "Connection: close\r\n" ) );
   // server_request_keep_alive_check( &con );
   // con.response.status_code = 200;
   // con.response.body        = "bye";
   //
   // http_server_set_default_headers_for_response( &con );
   // http_response_serialize( &con.response, &con.buf );
   // TEST_ASSERT_NOT_NULL( strstr( con.buf.data, "Connection: close" ) );
   TEST_IGNORE_MESSAGE(
       "TODO: send 'Connection: close' whenever is_keep_alive is false" );
}

//------------------------------------------------------------------------------
// Connection is a token list (RFC 7230 section 6.1) and may legally name
// hop-by-hop header names. Matching by substring means a header such as
//   Connection: X-Close-Me
// contains "close" and is misread as a close request. Token-aware matching
// (split on ',', trim OWS, compare whole tokens) fixes it.
void test_ka_connection_token_not_substring( void )
{
   // TEST_ASSERT_TRUE( keep_alive_for( REQ_11( "Connection: X-Close-Me\r\n" ) ) );
   // TEST_ASSERT_FALSE( keep_alive_for( REQ_10( "Connection: X-Keep-Alive-Hint\r\n" ) ) );
   TEST_IGNORE_MESSAGE(
       "TODO: match Connection tokens, not substrings -- 'X-Close-Me' "
       "currently reads as 'close'" );
}

//------------------------------------------------------------------------------
// Socket-level behaviour, not reachable from a unit test. Needs a running
// server driven over real sockets:
//
//   - idle keep-alive connection is reclaimed after the idle timeout
//   - a client that connects and sends nothing is reclaimed
//   - dribbling bytes must not refresh the header deadline (slowloris)
//   - two requests on one TCP connection both get responses
//   - server actually closes the socket after Connection: close
void test_ka_timeouts_need_integration_coverage( void )
{
   TEST_IGNORE_MESSAGE(
       "TODO: integration suite -- idle timeout, silent client, slowloris "
       "dribble, real connection reuse" );
}

//------------------------------------------------------------------------------
int main( void )
{
   g_kq = kqueue();

   UNITY_BEGIN();

   // HTTP/1.1 defaults
   RUN_TEST( test_ka_http11_default_is_persistent );
   RUN_TEST( test_ka_http11_connection_close );
   RUN_TEST( test_ka_http11_explicit_keepalive );
   RUN_TEST( test_ka_http11_keepalive_and_close_closes );
   RUN_TEST( test_ka_http11_close_after_other_headers );

   // HTTP/1.0 defaults
   RUN_TEST( test_ka_http10_default_is_close );
   RUN_TEST( test_ka_http10_explicit_keepalive );
   RUN_TEST( test_ka_http10_connection_close );

   // Case-insensitivity
   RUN_TEST( test_ka_http11_close_uppercase_value );
   RUN_TEST( test_ka_http11_close_mixed_case_value );
   RUN_TEST( test_ka_http10_keepalive_mixed_case_value );
   RUN_TEST( test_ka_http10_keepalive_uppercase_value );
   RUN_TEST( test_ka_header_name_case_insensitive );

   // Reuse / stale-flag regressions
   RUN_TEST( test_ka_reuse_persistent_then_close );
   RUN_TEST( test_ka_reuse_http11_then_http10 );
   RUN_TEST( test_ka_reuse_close_then_persistent );
   RUN_TEST( test_ka_check_overwrites_preexisting_flag );

   // connection_reset semantics
   RUN_TEST( test_ka_reset_clears_request_state_but_keeps_fd );
   RUN_TEST( test_ka_reset_drops_previous_headers );

   // Version handling
   RUN_TEST( test_ka_version_int_http11 );
   RUN_TEST( test_ka_version_int_http10 );
   RUN_TEST( test_ka_http11_missing_host_rejected );
   RUN_TEST( test_ka_http10_missing_host_allowed );
   RUN_TEST( test_ka_unsupported_versions_rejected );

   // Known-open issues
   RUN_TEST( test_ka_response_echoes_keepalive_for_http10 );
   RUN_TEST( test_ka_response_sends_connection_close_when_closing );
   RUN_TEST( test_ka_connection_token_not_substring );
   RUN_TEST( test_ka_timeouts_need_integration_coverage );

   if ( g_kq != -1 )
   {
      close( g_kq );
   }

   return UNITY_END();
}
