#pragma once

#include "Connection.h"

#define MAX_ROUTES 32

typedef void ( *RouteHandler_t )( Connection_t* con );

typedef struct
{
   char           method[ 8 ];
   int32_t        method_int;
   char           path[ 256 ];
   RouteHandler_t handler;
} Route_t;

typedef struct
{
   Route_t routes[ MAX_ROUTES ];
   int32_t count_routes;
} Router_t;

void router_add_route( Router_t* router, int32_t method, const char* path, RouteHandler_t handler );
RouteHandler_t router_find_route( Router_t* router, HttpRequest_t* request );
