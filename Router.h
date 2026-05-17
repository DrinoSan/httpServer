#pragma once

#include "Connection.h"

#define MAX_ROUTES 32

typedef void ( *RouteHandler_t )( Connection_t* con );

typedef enum
{
   ROUTE_MATCH_EXACT,
   ROUTE_MATCH_PREFIX,
} RouteMatchType_t;

typedef struct
{
   char             method[ 8 ];
   int32_t          method_int;
   char             path[ 256 ];
   RouteMatchType_t match_type;
   RouteHandler_t   handler;
   // @TODO add length so we dont have to calculate it always
} Route_t;

typedef struct
{
   Route_t routes[ MAX_ROUTES ];
   int32_t count_routes;
} Router_t;

void router_add_route( Router_t* router, int32_t method, const char* path,
                       RouteHandler_t handler );

RouteHandler_t router_find_route( Router_t* router, HttpRequest_t* request,
                                  Connection_t* con );

void router_add_static( Router_t* router, int32_t method, const char* path, RouteHandler_t handler );
