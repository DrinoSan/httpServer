#include <stdio.h>
#include <string.h>

#include "HttpRequest.h"

// clang-format off
// -----------------------------------------------------------------------------
const char *sand_http_method_to_string(uint32_t method)
{
    switch (method)
    {
        case SAND_HTTP_UNKNOWN:     return "UNKNOWN";
        case SAND_HTTP_GET:         return "GET";
        case SAND_HTTP_HEAD:        return "HEAD";
        case SAND_HTTP_POST:        return "POST";
        case SAND_HTTP_PUT:         return "PUT";
        case SAND_HTTP_DELETE:      return "DELETE";
        case SAND_HTTP_MKCOL:       return "MKCOL";
        case SAND_HTTP_COPY:        return "COPY";
        case SAND_HTTP_MOVE:        return "MOVE";
        case SAND_HTTP_OPTIONS:     return "OPTIONS";
        case SAND_HTTP_PROPFIND:    return "PROPFIND";
        case SAND_HTTP_PROPPATCH:   return "PROPPATCH";
        case SAND_HTTP_LOCK:        return "LOCK";
        case SAND_HTTP_UNLOCK:      return "UNLOCK";
        case SAND_HTTP_PATCH:       return "PATCH";
        case SAND_HTTP_TRACE:       return "TRACE";
        case SAND_HTTP_CONNECT:     return "CONNECT";
        case SAND_HTTP_ALL_METHODS: return "ALL";
        default:                    return "INVALID";
    }
}
// clang-format on

// -----------------------------------------------------------------------------
const sand_string_view_t*
http_request_find_header( const HttpRequest_t* request, const char* name )
{
   for ( int32_t i = 0; i < request->header_count; i++ )
   {
      if ( strcmp( request->headers[ i ].name, name ) == 0 )
      {
         return &request->headers[ i ].value;
      }
   }

   return NULL;
}

// -----------------------------------------------------------------------------
void http_request_showMe( const HttpRequest_t* request )
{
   printf( "=== HttpRequest ===\n" );
   printf( "method:         %s\n", request->method );
   printf( "method_int:     0x%08X\n", request->method_int );
   printf( "path:           %s\n", request->path );
   printf( "version:        %s\n", request->version );
   printf( "version_int:    %d\n", request->version_int );
   printf( "http_major:     %d\n", request->http_major );
   printf( "http_minor:     %d\n", request->http_minor );
   printf( "state:          %d\n", request->state );
   printf( "complex_uri:    %d\n", request->complex_uri );
   printf( "content_length: %d\n", request->content_length );
   printf( "body:           %s\n", request->body ? request->body : "(null)" );
   printf( "request_start:  %p\n", ( void* ) request->request_start );
   printf( "request_end:    %p\n", ( void* ) request->request_end );
   printf( "schema_start:   %p\n", ( void* ) request->schema_start );
   printf( "schema_end:     %p\n", ( void* ) request->schema_end );
   printf( "method_end:     %p\n", ( void* ) request->method_end );
   printf( "uri_start:      %p\n", ( void* ) request->uri_start );
   printf( "uri_end:        %p\n", ( void* ) request->uri_end );
   printf( "header_count:   %d\n", request->header_count );
   for ( int32_t i = 0; i < request->header_count; i++ )
   {
      printf( "  [%d] %s: %.*s\n", i, request->headers[ i ].name,
              ( int ) request->headers[ i ].value.size,
              request->headers[ i ].value.data );
   }
   printf( "===================\n" );
}

// -----------------------------------------------------------------------------
void http_request_reset_headers( HttpRequest_t* request )
{
   for( size_t i = 0; i < request->header_count; ++i )
   {
      HttpHeader_t* header = &request->headers[ i ];
      memset( header->name, 0, MAX_HEADER_NAME_LEN );
      sand_string_view_reset( &header->value );
   }

   request->header_count = 0;
}
