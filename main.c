#include <stdio.h>

#include "Connection.h"
#include "Router.h"
#include "Server.h"

void handle_index( Connection_t* con )
{
   http_response_set_header( &con->response, "Content-Type", "text/html" );
   con->response.status_code = 200;
   con->response.body        = "<h1>Hello</h1>";
}

int main()
{
   Server_t server;
   server_create( &server );

   router_add_route( &server.router, SAND_HTTP_GET, "/home", handle_index );
   server_serve_static_files( &server, "home", "/static/" );

   server_start( &server );

   server_destroy( &server );
}
