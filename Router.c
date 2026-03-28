#include <string.h>

#include "Log.h"
#include "Router.h"

//------------------------------------------------------------------------------
void handle_404_not_found( Connection_t* con );

//------------------------------------------------------------------------------
void router_add_route( Router_t* router, int32_t method, const char* path,
                       RouteHandler_t handler )
{
   // Example
   // SAND_HTTP_ALL_METHODS:   0x0001FFFF
   // SAND_HTTP_GET:           0x00000002

   // IN BITS:
   // SAND_HTTP_ALL_METHODS:  00011111
   // SAND_HTTP_GET:          00000010
   //
   // ~SAND_HTTP_ALL_METHODS: 11100000
   // SAND_HTTP_GET:          00000010
   //
   // Calculation
   // ~SAND_HTTP_ALL_METHODS
   // &
   // SAND_HTTP_GET
   //
   // 11100000
   // &
   // 00000010
   // 00000000 -> Result is zero meaning its a valid http get that we support

   if ( method & ~SAND_HTTP_ALL_METHODS )
   {
      LOG_WARN( "Unknown Method, will not create route" );
      return;
   }

   if ( strlen( path ) >= sizeof( router->routes[ 0 ].path ) )
   {
      LOG_WARN( "Path %s size %d to long, max size %d - Route not added", path,
                strlen( path ), sizeof( router->routes[ 0 ].path ) );
      return;
   }

   if ( router->count_routes >= MAX_ROUTES )
   {
      LOG_WARN( "Max amount of routes reached, route will not be added - Max "
                "routes %d",
                MAX_ROUTES );
      return;
   }

   Route_t* route    = &router->routes[ router->count_routes ];
   route->method_int = method;

   strncpy( route->path, path,
            sizeof( route->path ) - 1 );   // -1 so i have space for \0 memset
                                           // already set all bytes to \0
   route->handler = handler;

   router->count_routes++;
}

//------------------------------------------------------------------------------
RouteHandler_t router_find_route( Router_t* router, HttpRequest_t* request )
{
   for ( int32_t i = 0; i < router->count_routes; i++ )
   {
      Route_t* route = &router->routes[ i ];
      LOG_WARN( "request->uri_view: %.*s route path: %s",
                ( int ) request->uri_view.size, request->uri_view.data,
                route->path );
      if ( request->uri_view.size != strlen( route->path ) )
      {
         continue;
      }

      if ( route->method_int == request->method_int &&
           memcmp( route->path, request->uri_view.data,
                   request->uri_view.size ) == 0 )
      {
         return route->handler;
      }
   }

   return handle_404_not_found;
}

//------------------------------------------------------------------------------
void handle_404_not_found( Connection_t* con )
{
   con->response.status_code = 404;
   strcpy( con->response.status_text, "Not Found" );
   con->response.body =
       "<h1>Sorry, the page you are asking for is not registered</h1>";
}
