#include <stdio.h>

#include "Server.h"

int main()
{
   Server_t server;
   server_create( &server );

   while( true )
   {
      int clientSocket = socketHandler_acceptConnection( &server.socketHandler );
      printf( "Received Client Socket <%d>\n", clientSocket );
   }


   server_destroy( &server );
}
