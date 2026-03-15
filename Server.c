#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // for memset
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Server.h"

#define BUFFER_SIZE 8192

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

      // Only the main thread is accesing next_worker so we safe
      int32_t worker_idx  = server->next_worker;
      server->next_worker = ( server->next_worker + 1 ) % NUM_WORKERS;

      struct kevent change;
      EV_SET( &change, clientFD, EVFILT_READ, EV_ADD, 0, 0, NULL );
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
   char buffer[ BUFFER_SIZE + 1 ];
   // I need to cast it back into 32bit from a void*
   int32_t kqueueFD = ( int32_t ) ( intptr_t ) kqueueFD_;

   struct kevent events[ 64 ];
   while ( true )
   {
      int n = kevent( kqueueFD, NULL, 0, events, 64, NULL );
      printf( "n from k events %d\n", n );
      memset( buffer, 0, BUFFER_SIZE + 1 );

      for ( int32_t i = 0; i < n; i++ )
      {
         // Handle new client connection
         printf( "Got new Socket/client connection for socket %lu\n",
                 events[ i ].ident );

         int32_t bytes_read = recv( events[ i ].ident, buffer, BUFFER_SIZE, 0 );
         if ( bytes_read == 0 )
         {
            // Client disconnected — remove from kqueue and close
            close( events[ i ].ident );
         }
         else if ( bytes_read < 0 )
         {
            if ( errno != EAGAIN )
            {
               close( events[ i ].ident );
            }
            // EAGAIN just means no data right now, ignore
         }

         printf( "Read bytes %d\n", bytes_read );
         printf( "Buffer\n\t%s\n", buffer );
      }
   }

   // Kevent handling
}
