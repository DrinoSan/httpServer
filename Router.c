#include "Router.h"
#include "Log.h"

void router_add_route( Router_t* router, const char* method, const char* path,
                       RouteHandler_t handler )
{
   LOG_WARN( "Method %s size %d to long, max size 8 - Route not added", method,
             sizeof( method ) );
   return;
}

RouteHandler_t router_find_route( Router_t* router, const char* method,
                                  const char* path )
{
}
