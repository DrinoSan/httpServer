#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <unistd.h>

#include "Connection.h"
#include "Log.h"

//------------------------------------------------------------------------------
Connection_t* connection_create_heap( int32_t fd )
{
   Connection_t* con = ( Connection_t* ) malloc( sizeof( Connection_t ) );
   if ( con == NULL )
   {
      return NULL;
   }

   memset( con, 0, sizeof( Connection_t ) );

   sand_string_create( &con->buf );

   sand_string_create( &con->buf_for_error_405 );

   con->fd            = fd;
   con->state         = CONN_READING_HEADERS;
   con->is_keep_alive = false;

   return con;
}

//------------------------------------------------------------------------------
void connection_destroy( Connection_t* con )
{
   LOG_WARN( "Freeing Connection of FD %d and closing socket\n", con->fd );

   // Free response header values
   for ( int i = 0; i < con->response.header_count; i++ )
   {
      sand_string_destroy( &con->response.headers[ i ].value );
   }

   // Removing registered timer for connection
   struct kevent timer;
   EV_SET( &timer, con->fd, EVFILT_TIMER, EV_DELETE, 0, 0, NULL );
   kevent( con->kqueueFd, &timer, 1, NULL, 0, NULL );

   // Closing socket after i remove the timer otherwise on heave load the socket
   // could be reused
   close( con->fd );

   sand_string_destroy( &con->buf );
   sand_string_destroy( &con->buf_for_error_405 );
   free( con );
}

//------------------------------------------------------------------------------
void connection_reset( Connection_t* con )
{
   LOG_INFO( "Reseting Connection keep alive is activated" );

   http_response_reset_headers( &con->response );
   http_request_reset_headers( &con->request );

   // reset for next request
   sand_string_clear( &con->buf );
   sand_string_clear( &con->buf_for_error_405 );

   con->state         = CONN_READING_HEADERS;
   con->bytes_read    = 0;
   con->header_len    = 0;
   con->is_keep_alive = false;
   memset( con->buffer, 0, BUFFER_SIZE );

   // Removing registered timer for connection
   struct kevent timer;
   EV_SET( &timer, con->fd, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, 10000, con );
   kevent( con->kqueueFd, &timer, 1, NULL, 0, NULL );
}
