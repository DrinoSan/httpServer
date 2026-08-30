#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // for memset
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Connection.h"
#include "HttpParser.h"
#include "Log.h"
#include "Router.h"
#include "Sand_string.h"
#include "Server.h"

// SandLib
#include "sand_file.h"

//======================PRIVATE INTERFACE DECLARATION==========================
void server_setup_worker( Server_t* server );
void server_handle_parsing_error( Connection_t* con, ParseResult_t result );
void server_serialize_and_send_response( Connection_t* response );

// Static file handler
void server_serve_static_files_handler( Connection_t* con );

//------------------------------------------------------------------------------
void sigint_handler( int sig );

// Thread func
void* server_start_worker_event_loop( void* workerArgs );

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//======================PUBLIC INTERFACE IMPLEMENTATION=========================
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void server_create( Server_t* server )
{
   // Starting with a clean struct
   memset( server, 0, sizeof( *server ) );

   // Getting socket for server
   socketHandler_create( &server->socketHandler, AF_INET, SOCK_STREAM, 0 );

   // Binding to socket and start listening
   socketHandler_init( &server->socketHandler, "", 8080, 10 );

   server_setup_worker( server );
}

//------------------------------------------------------------------------------
void server_destroy( Server_t* server )
{
   LOG_INFO( "Destroying Server" );
   socketHandler_destroy( &server->socketHandler );
}

//------------------------------------------------------------------------------
void server_start( Server_t* server )
{
   struct sigaction sa = {
       .sa_handler = sigint_handler,
       .sa_flags   = 0,
   };
   sigemptyset( &sa.sa_mask );

   while ( true )
   {
      int clientFD = socketHandler_acceptConnection( &server->socketHandler );
      int flags    = fcntl( clientFD, F_GETFL );
      fcntl( clientFD, F_SETFL,
             flags | O_NONBLOCK );   // Set non-blocking so recv/send never
                                     // block the worker's event loop
      if ( sigaction( SIGINT, &sa, NULL ) == -1 )
      {
         perror( "sigaction" );
         exit( 1 );
      }

      // Here i need to setup the connection
      Connection_t* connection = connection_create_heap( clientFD );
      if ( connection == NULL )
      {
         close( clientFD );
         continue;
      }

      // Only the main thread is accesing next_worker so we safe
      int32_t worker_idx  = server->next_worker;
      server->next_worker = ( server->next_worker + 1 ) % NUM_WORKERS;

      connection->kqueueFd = server->worker_kqueue_fds[ worker_idx ];

      // Registering the new client in our kqueue
      struct kevent change;
      EV_SET( &change, clientFD, EVFILT_READ, EV_ADD, 0, 0, connection );
      kevent( server->worker_kqueue_fds[ worker_idx ], &change, 1, NULL, 0,
              NULL );

      LOG_INFO( "Received Client Socket <%d> Adding it to KQueue of worker %d",
                clientFD, worker_idx );
   }

   // Gracefully joining the threads
   for ( int32_t i = 0; i < NUM_WORKERS; i++ )
   {
      pthread_join( server->server_worker_thread[ i ], NULL );
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//======================PRIVATE INTERFACE IMPLEMENTATION========================
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void server_setup_worker( Server_t* server )
{
   for ( int32_t i = 0; i < NUM_WORKERS; i++ )
   {
      int32_t kq = -1;
      if ( ( kq = kqueue() ) == -1 )
      {
         perror( "Init of kqueue failed" );
         exit( EXIT_FAILURE );
      }

      server->worker_kqueue_fds[ i ] = kq;
   }

   for ( int32_t i = 0; i < NUM_WORKERS; i++ )
   {
      LOG_INFO( "Worker %d aquired kqueue fd %d", i,
                server->worker_kqueue_fds[ i ] );
   }

   for ( int32_t i = 0; i < NUM_WORKERS; i++ )
   {
      pthread_t thread;

      WorkerArgs_t* args = &server->worker_args[ i ];
      args->kqueue_fd    = server->worker_kqueue_fds[ i ];
      args->server       = server;

      pthread_create( &thread, NULL, server_start_worker_event_loop,
                      ( void* ) args );
      //( void* ) ( intptr_t )
      //    server->worker_kqueue_fds[ i ] );   // Need to cast
      //    to intptr_t to
      // get a 64bit representation
      server->server_worker_thread[ i ] = thread;
   }
}

//------------------------------------------------------------------------------
void* server_start_worker_event_loop( void* args )
{
   // I need to cast it back into 32bit from a void*
   WorkerArgs_t* workerArgs = ( WorkerArgs_t* ) args;
   // int32_t       kqueueFD   = ( int32_t ) ( intptr_t ) workerArgs->kqueue_fd;
   int32_t   kqueueFD = workerArgs->kqueue_fd;
   Server_t* server   = workerArgs->server;

   const int32_t num_events = 64;
   struct kevent events[ num_events ];
   while ( true )
   {
      int n = kevent( kqueueFD, NULL, 0, events, num_events, NULL );

      for ( int32_t i = 0; i < n; i++ )
      {
         // Handle new client connection
         Connection_t* con = ( Connection_t* ) events[ i ].udata;
         // ident and con->fd is the same
         LOG_INFO(
             "Got new Socket/client connection for socket %lu udata fd %d",
             events[ i ].ident, con->fd );

         if ( events[ i ].filter == EVFILT_TIMER )
         {
            // Timeout - Client sleeps or is bad. Timout register is done at
            // Set when state changes to CONN_READING_BODY
            LOG_INFO( "Got timeout for fd <%d>", con->fd );
            connection_destroy( con );
            continue;
         }
         else if ( events[ i ].filter == EVFILT_READ )
         {
            // Checking if client sent more data than BUFFER_SIZE
            // if my buffer is exhausted recv would also return 0 but in this
            // case it does not mean that i got a FIN
            if ( con->bytes_read == BUFFER_SIZE )
            {
               // @TODO
               if ( con->state == CONN_READING_HEADERS )
               {
                  // Return 431 error
                  // 431 Request Header Fields Too Large.
               }
               else if ( con->state == CONN_READING_BODY )
               {
                  // Return 413 error
                  // 413 Content Too Large
               }
            }

            // 10 to check if i really fill the same buffer should be
            // BUFFER_SIZE - con->bytes_read to avoid buffer overflow
            int32_t bytes_read = recv( con->fd, con->buffer + con->bytes_read,
                                       BUFFER_SIZE - con->bytes_read, 0 );

            if ( bytes_read == 0 )
            {
               // Client disconnected — remove from kqueue and close
               LOG_INFO( "Got 0 bytes read for fd <%d>", con->fd );
               connection_destroy( con );
               continue;
            }
            else if ( bytes_read < 0 )
            {
               if ( errno != EAGAIN )
               {
                  LOG_WARN( "EAGIAIN ERROR" );
                  connection_destroy( con );
                  continue;
               }

               // EAGAIN just means no data right now, ignore
               continue;
            }

            con->bytes_read = con->bytes_read + bytes_read;

            if ( con->state == CONN_READING_HEADERS )
            {
               char* end = strstr( con->buffer, "\r\n\r\n" );
               if ( end == NULL )
               {
                  // Did not receive the full headers. Setting timer just in
                  // case i never get something back
                  struct kevent timer;
                  EV_SET( &timer, con->fd, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0,
                          10000, con );
                  kevent( kqueueFD, &timer, 1, NULL, 0, NULL );
                  continue;
               }

               // At this point i know i read all headers now content can be
               // read the +4 is because of "\r\n\r\n"
               con->header_len = ( end - con->buffer ) + 4;

               // Need to parse the headers
               ParseResult_t result = http_parser_parse_request(
                   con->buffer, con->header_len, &con->request );

               if ( result != PARSE_OK )
               {
                  server_handle_parsing_error( con, result );
                  connection_destroy( con );
                  continue;
               }

               // Setting keep_alive flag depending on headers and/or http
               // version
               server_request_keep_alive_check( con );

               const sand_string_view_t* length_value_ptr =
                   http_request_find_header( &con->request, "content-length" );

               const sand_string_view_t* chunk_value_ptr =
                   http_request_find_header( &con->request,
                                             "transfer-encoding" );
               // If no content_length provided we dont
               // need to check the body
               if ( length_value_ptr != NULL )
               {
                  const char* length_value    = length_value_ptr->data;
                  con->request.content_length = atoi( length_value );
                  con->state                  = CONN_READING_BODY;

                  int32_t body_received = con->bytes_read - con->header_len;
                  if ( body_received >= con->request.content_length )
                  {
                     // Full request received — handle it
                     con->state = CONN_SENDING_RESPONSE;
                  }
                  else
                  {
                     // I did not get the full body so i can set a timmer for
                     // further recv calls
                     // This is for clients fall asleep or other bad stuff
                     struct kevent timer;
                     EV_SET( &timer, con->fd, EVFILT_TIMER, EV_ADD | EV_ONESHOT,
                             0, 10000, con );
                     kevent( kqueueFD, &timer, 1, NULL, 0, NULL );
                  }
               }
               else
               {
                  // No body expected — request is complete
                  con->state = CONN_SENDING_RESPONSE;
               }
            }
            else if ( con->state == CONN_READING_BODY )
            {
               int32_t body_received = con->bytes_read - con->header_len;
               if ( body_received >= con->request.content_length )
               {
                  // Full request received — handle it
                  con->state = CONN_SENDING_RESPONSE;
               }
            }
         }

         if ( con->state == CONN_SENDING_RESPONSE )
         {
            RouteHandler_t handler =
                router_find_route( &server->router, &con->request, con );

            // If no route was registered the router returns
            // handle_404_not_found
            handler( con );
            if ( con->response.status_code == 0 )
            {
               con->response.status_code = 200;
            }

            server_serialize_and_send_response( con );

            if ( con->is_keep_alive == false )
            {
               // If keepalive is not set its safe to destroy the connection
               connection_destroy( con );
               continue;
            }

            connection_reset( con );
         }
      }
   }
}

//------------------------------------------------------------------------------
void server_request_keep_alive_check( Connection_t* con )
{
   // Default case always set. I think its alright because i save the http
   // version in version_int
   const sand_string_view_t* connection_header_value =
       http_request_find_header( &con->request, "connection" );

   con->is_keep_alive = false;
   if ( con->request.version_int == 1001 )
   {
      con->is_keep_alive = true;
      if ( connection_header_value != NULL &&
           sand_string_view_has_substr_no_case( connection_header_value,
                                                "close" ) )
      {
         con->is_keep_alive = false;
      }
   }
   else if ( con->request.version_int == 1000 )
   {
      con->is_keep_alive = false;
      if ( connection_header_value != NULL &&
           sand_string_view_has_substr_no_case( connection_header_value,
                                                "keep-alive" ) )
      {
         con->is_keep_alive = true;
      }
   }
}

//------------------------------------------------------------------------------
void server_handle_parsing_error( Connection_t* con, ParseResult_t result )
{
   Sand_string_t buf;
   sand_string_create( &buf );

   switch ( result )
   {
   case PARSE_ERROR_MALFORMED_REQUEST_LINE:
   {
      con->response.status_code = 400;
      break;
   }

   case PARSE_ERROR_INVALID_HEADERS:
   {
      con->response.status_code = 431;
      break;
   }

   case PARSE_ERROR_PATH_TOO_LONG:
   {
      con->response.status_code = 431;
      break;
   }

   case PARSE_ERROR_TOO_MANY_HEADERS:
   {
      con->response.status_code = 431;
      break;
   }

   case PARSE_ERROR_MISSING_HOST:
   {
      con->response.status_code = 400;
      break;
   }

   case PARSE_ERROR_METHOD_WITHOUT_CL_OR_TRANSFER_ENCODING:
   {
      con->response.status_code = 411;
      break;
   }

   default:
   {
      // assert because if i forgot something i need to be reminded :)
      assert( false && "Your forgot to handle a parsing error" );
   }
   };

   http_response_serialize( &con->response, &buf );
   send( con->fd, buf.data, buf.size, 0 );
   sand_string_destroy( &buf );
}

//------------------------------------------------------------------------------
void server_serialize_and_send_response( Connection_t* con )
{
   http_server_set_default_headers_for_response( con );
   // @TODO
   // HTTP/1.0 responses must echo "Connection: keep-alive" when the
   // connection is being kept open. A 1.0 client treats a missing
   // Connection header as "server is closing" and will not reuse the
   // socket -- so the connection sits idle until the timer kills it.
   // Also send "Connection: close" explicitly whenever is_keep_alive
   // is false, so the client stops pipelining into a socket we are
   // about to shut down.
   http_response_serialize( &con->response, &con->buf );

   send( con->fd, con->buf.data, con->buf.size, 0 );
   LOG_WARN( "Sending response buffer:\n%s\n", con->buf.data );
   sand_string_destroy( &con->buf );
}

//------------------------------------------------------------------------------
void http_server_set_default_headers_for_response( Connection_t* con )
{
   // @TODO
   // Keep-alive is computed here, but this function returns early when
   // response.body == NULL (204, 304, and every error path that sets no
   // body). Those responses never reach server_request_keep_alive_check,
   // and connection_reset does not clear is_keep_alive -- so the flag
   // carries over from the PREVIOUS request on this connection.
   // Fix: compute is_keep_alive right after http_parser_parse_request
   // succeeds (version + headers are both known there) and let the
   // response path only read the flag, never set it.
   if ( con->response.body == NULL )
   {
      sand_string_append( &con->buf, "Content-Length: 0\r\n\r\n" );
      return;
   }

   // ============= BEGIN Settting default content length =============
   // Getting content-length size
   char content_length[ 64 ];
   int  body_len = strlen( con->response.body ) + 1;   // +1 for the "\n"

   snprintf( content_length, sizeof( content_length ), "%d", body_len );

   http_response_set_header( &con->response, "Content-Length", content_length );
   // ============= END Settting default content length =============
}

//------------------------------------------------------------------------------
void server_serve_static_files_handler( Connection_t* con )
{
   LOG_WARN( "Called serving static files " );

   // parse prefix away, i only support single prefix meaning /static/home.html
   // is valid But /static/secondStatic/home.html would not be supported So now
   // i want to parse away the prefix

   char* c = con->request.uri_view.data;
   // Skip the first '/'
   c++;

   for ( ; c != con->request.uri_view.data + con->request.uri_view.size; c++ )
   {
      if ( *c == '/' )
      {
         c++;
         break;
      }
   }

   sand_string_view_t target_view;
   target_view.data = c;
   target_view.size =
       ( con->request.uri_view.data + con->request.uri_view.size ) - c;

   // to hold file name
   char file_name[ 100 ];

   memcpy( file_name, target_view.data, target_view.size );
   file_name[ target_view.size ] = '\0';

   char file_extension[ 10 ] = { 0 };
   for ( size_t i = 0; i < target_view.size; i++ )
   {
      if ( target_view.data[ i ] == '.' )
      {
         i++;
         if ( target_view.size - i > 10 )
         {
            assert( false && "Extension to long" );
         }

         memcpy( file_extension, target_view.data + i, target_view.size - i );
      }
   }

   size_t extension_length = strlen( file_extension );

   // @TODO: implement in sandlib
   // sand_string_view_compare() or is_equal
   const char* mime = NULL;
   for ( size_t i = 0; i < sizeof( mime_types ) / sizeof( mime_types[ 0 ] );
         i++ )
   {
      if ( extension_length == mime_types[ i ].len &&
           memcmp( file_extension, mime_types[ i ].ext, extension_length ) ==
               0 )
      {
         mime = mime_types[ i ].mime;
         // http_response_set_header( &con->response, "Content-Type",
         // mime_types[ i ].mime );
         break;
      }
   }

   // @TODO: files should not be read everytime on request. We should hold them
   // prepared
   sand_file_t file;
   sand_file_create( &file );
   sand_file_open_and_read( &file, file_name );
   if ( file.content.size == 0 )
   {
      LOG_WARN( "Could not open file" );
   }
   else
   {
      LOG_INFO( "Data Read \n[\n%s]", file.content.data );
   }

   if ( mime != NULL )
   {
      // I know i could directly set that inside the for loop above but i wanted
      // to have all the setting of data response in one part
      http_response_set_header( &con->response, "Content-Type", mime );
   }

   con->response.status_code = 200;
   con->response.body        = file.content.data;
}

//------------------------------------------------------------------------------
void server_serve_static_files( Server_t* server, const char* file_path,
                                const char* endpoint )
{
   router_add_static( &server->router, SAND_HTTP_GET, endpoint,
                      server_serve_static_files_handler );
}

//------------------------------------------------------------------------------
void sigint_handler( int sig )
{
   ( void ) sig;
   const char msg[] = "Server shutting down!\n";
   write( 1, msg, sizeof( msg ) - 1 );
   fflush( stdout );
   exit( 1 );
}
