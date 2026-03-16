#include <stdio.h>

#include "Server.h"

int main()
{
   Server_t server;
   server_create( &server );

   server_start( &server );

   server_destroy( &server );
}
