#include <stdio.h>

#include "Server.h"
#include "Connection.h"
#include "Router.h"

void handle_index( Connection_t* con )
{
   con->response.status_code = 200;
   strcpy( con->response.status_text, "OK" );
   con->response.body = "<h1>Hello</h1>";
   con->response.body_len = 14;
}

int main()
{
   Server_t server;
   server_create( &server );

   router_add_route( &server.router, "GET", "/home", handle_index );

   server_start( &server );

   server_destroy( &server );
}
