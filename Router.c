#include <string.h>

#include "Log.h"
#include "Router.h"

void router_add_route( Router_t* router, const char* method, const char* path,
                       RouteHandler_t handler )
{
   if ( strlen( method ) >= sizeof( router->routes[ 0 ].method ) )
   {
      LOG_WARN( "Method %s size %d to long, max size %d - Route not added",
                method, strlen( method ),
                sizeof( router->routes[ 0 ].method ) );
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

   Route_t* route = &router->routes[ router->count_routes ];
   strncpy( route->method, method,
            sizeof( route->method ) - 1 );   // -1 so i have space for \0 memset
                                             // already set all bytes to \0
   strncpy( route->path, path,
            sizeof( route->path ) - 1 );   // -1 so i have space for \0 memset
                                           // already set all bytes to \0
   route->handler = handler;

   router->count_routes++;
}

RouteHandler_t router_find_route( Router_t* router, const char* method,
                                  const char* path )
{
   for ( int32_t i = 0; i < router->count_routes; i++ )
   {
      Route_t* route = &router->routes[ i ];
      if ( strcmp( route->method, method ) == 0 &&
           strcmp( route->path, path ) == 0 )
      {
         return route->handler;
      }
   }

   return NULL;
}
