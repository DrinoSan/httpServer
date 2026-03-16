#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // for memset
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Connection.h"
#include "HttpParser.h"
#include "Server.h"

//======================PRIVATE INTERFACE DECLARATION==========================
void server_setup_worker( Server_t* server );

// Thread func
void* server_start_worker_event_loop( void* kqueueFD_ );

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
   printf( "Destroying Server\n" );
   socketHandler_destroy( &server->socketHandler );
}

//------------------------------------------------------------------------------
void server_start( Server_t* server )
{
   while ( true )
   {
      int clientFD = socketHandler_acceptConnection( &server->socketHandler );
      int flags    = fcntl( clientFD, F_GETFL );
      fcntl( clientFD, F_SETFL,
             flags | O_NONBLOCK );   // Set non-blocking so recv/send never
                                     // block the worker's event loop

      // Here i need to setup the connection
      Connection_t* connection = connection_create_heap( clientFD );

      // Only the main thread is accesing next_worker so we safe
      int32_t worker_idx  = server->next_worker;
      server->next_worker = ( server->next_worker + 1 ) % NUM_WORKERS;

      // Registering the new client in our kqueue
      struct kevent change;
      EV_SET( &change, clientFD, EVFILT_READ, EV_ADD, 0, 0, connection );
      kevent( server->worker_kqueue_fds[ worker_idx ], &change, 1, NULL, 0,
              NULL );

      printf( "Received Client Socket <%d> Adding it to KQueue of worker %d\n",
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
      printf( "Worker %d aquired kqueue fd %d\n", i,
              server->worker_kqueue_fds[ i ] );
   }

   for ( int32_t i = 0; i < NUM_WORKERS; i++ )
   {
      pthread_t thread;
      pthread_create(
          &thread, NULL, server_start_worker_event_loop,
          ( void* ) ( intptr_t )
              server->worker_kqueue_fds[ i ] );   // Need to cast to intptr_t to
                                                  // get a 64bit representation
      server->server_worker_thread[ i ] = thread;
   }
}

//------------------------------------------------------------------------------
void* server_start_worker_event_loop( void* kqueueFD_ )
{
   // I need to cast it back into 32bit from a void*
   int32_t kqueueFD = ( int32_t ) ( intptr_t ) kqueueFD_;

   const int32_t num_events = 64;
   struct kevent events[ num_events ];
   while ( true )
   {
      int n = kevent( kqueueFD, NULL, 0, events, num_events, NULL );
      printf( "n from k events %d\n", n );

      for ( int32_t i = 0; i < n; i++ )
      {
         // Handle new client connection
         Connection_t* con = ( Connection_t* ) events[ i ].udata;
         printf(
             "Got new Socket/client connection for socket %lu udata fd %d\n",
             events[ i ].ident, con->fd );

         if ( events[ i ].filter == EVFILT_TIMER )
         {
            // Timeout - Client sleeps or is bad. Timout register is done at
            // Set when state changes to CONN_READING_BODY
            connection_destroy( con );
            continue;
         }
         else if ( events[ i ].filter == EVFILT_READ )
         {
            // 10 to check if i really fill the same buffer should be
            // BUFFER_SIZE - con->bytes_read to avoid buffer overflow
            int32_t bytes_read =
                recv( con->fd, con->buffer + con->bytes_read, 10, 0 );

            if ( bytes_read == 0 )
            {
               // Client disconnected — remove from kqueue and close
               connection_destroy( con );
               continue;
            }
            else if ( bytes_read < 0 )
            {
               if ( errno != EAGAIN )
               {
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
                  // Did not receive the full headers
                  continue;
               }

               // At this point i know i read all headers now content can be
               // read
               con->header_len = ( end - con->buffer ) + 4;

               // Need to parse the headers
               http_parser_parse_headers( con->buffer, con->header_len,
                                          &con->request );

               const char* length_value =
                   http_request_find_header( &con->request, "content-length" );
               // If its a GET request or no content_length provided we dont
               // need to check the body
               if ( strcmp( con->request.method, "GET" ) != 0 &&
                    length_value != NULL )
               {
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

                  struct kevent timer;
                  EV_SET( &timer, con->fd, EVFILT_TIMER, EV_DELETE, 0, 0,
                          NULL );
                  kevent( kqueueFD, &timer, 1, NULL, 0, NULL );
               }
            }
         }

         if ( con->state == CONN_SENDING_RESPONSE )
         {
            printf( "Read bytes %d\n", con->bytes_read );
            printf( "Buffer\n\t%s\n", con->buffer );
            const char* response =
                "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
            send( con->fd, response, strlen( response ), 0 );
            connection_destroy( con );
            continue;

            // const char* conn_header =
            //     http_request_find_header( &con->request, "connection" );
            // if ( conn_header != NULL && strcmp( conn_header, "close" ) == 0 )
            //{
            //    connection_destroy( con );
            //    continue;
            // }

            //// Keep-alive: reset for next request
            // con->state      = CONN_READING_HEADERS;
            // con->bytes_read = 0;
            // con->header_len = 0;
            // memset( &con->request, 0, sizeof( HttpRequest_t ) );
            // memset( con->buffer, 0, BUFFER_SIZE );

            // struct kevent timer;
            // EV_SET( &timer, con->fd, EVFILT_TIMER, EV_DELETE, 0, 0, NULL );
            // kevent( kqueueFD, &timer, 1, NULL, 0, NULL );
         }
      }
   }
}
