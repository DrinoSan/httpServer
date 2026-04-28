#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Connection.h"
#include "Log.h"

//------------------------------------------------------------------------------
Connection_t* connection_create_heap( int32_t fd )
{
   Connection_t* con = ( Connection_t* ) malloc( sizeof( Connection_t ) );
   memset( con, 0, sizeof( Connection_t ) );

   sand_string_create( &con->buf );
   memset( &con->response, 0, MAX_HEADERS );

   con->fd    = fd;
   con->state = CONN_READING_HEADERS;

   return con;
}

//------------------------------------------------------------------------------
void connection_destroy( Connection_t* con )
{
   LOG_WARN( "Freeing Connection if FD %d and closing socket\n", con->fd );
   close( con->fd );

   // Free response header values
   for ( int i = 0; i < con->response.header_count; i++ )
   {
      sand_string_destroy( &con->response.headers[ i ].value );
   }

   sand_string_destroy( &con->buf );
   free( con );
}
