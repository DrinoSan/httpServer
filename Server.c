#include <stdio.h>
#include <stdlib.h>
#include <string.h>   // for memset
#include <sys/event.h>

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
      int clientSocket =
          socketHandler_acceptConnection( &server->socketHandler );
      printf( "Received Client Socket <%d>\n", clientSocket );
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

void server_setup_worker( Server_t* server )
{
   // Setting up kqueues for workers
   // First zero it out
   memset( server->worker_kqueue_fds, 0, sizeof( server->worker_kqueue_fds ) );
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
      pthread_create( &thread, NULL, server_start_worker_event_loop,
                      ( void* ) ( intptr_t ) server->worker_kqueue_fds[ i ] ); // Need to cast to intptr_t to get a 64bit representation
      server->server_worker_thread[ i ] = thread;
   }
}

void* server_start_worker_event_loop( void* kqueueFD_ )
{
   int32_t kqueueFD = (int32_t)(intptr_t)kqueueFD_; // I need to cast it back into 32bit from a void*
   // Kevent handling
}
