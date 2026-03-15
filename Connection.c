#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "Connection.h"

//------------------------------------------------------------------------------
Connection_t* connection_create_heap( int32_t fd )
{
   Connection_t* con = (Connection_t*)malloc( sizeof( Connection_t ) );
   memset( con, 0, sizeof( Connection_t ) );

   con->fd = fd;
   con->state = CONN_READING_HEADERS;

   return con;
}

//------------------------------------------------------------------------------
void connection_destroy( Connection_t* con )
{
   printf( "Freeing Connection if FD %d and closing socket\n", con->fd );
   close( con->fd );
   free( con );
}
