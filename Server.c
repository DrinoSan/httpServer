#include <stdio.h>

#include "Server.h"

void server_create( Server_t* server )
{
   // Getting socket for server
   socketHandler_create( &server->socketHandler, AF_INET, SOCK_STREAM, 0 );

   // Binding to socket and start listening
   socketHandler_init( &server->socketHandler, "", 8080, 10 );
}

void server_destroy( Server_t* server )
{
   printf( "Destroying Server\n" );
   socketHandler_destroy( &server->socketHandler );
}
