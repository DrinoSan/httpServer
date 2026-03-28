#include <stdio.h>
#include <string.h>


#include "HttpRequest.h"

const char* http_request_find_header(const HttpRequest_t* request, const char* name)
{
   for( int32_t i = 0; i < request->header_count; i++ )
   {
      if( strcmp( request->headers[ i ].name, name ) == 0 )
      {
         return request->headers[ i ].value;
      }
   }

   return NULL;
}

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
   printf( "request_start:  %p\n", (void*)request->request_start );
   printf( "request_end:    %p\n", (void*)request->request_end );
   printf( "schema_start:   %p\n", (void*)request->schema_start );
   printf( "schema_end:     %p\n", (void*)request->schema_end );
   printf( "method_end:     %p\n", (void*)request->method_end );
   printf( "uri_start:      %p\n", (void*)request->uri_start );
   printf( "uri_end:        %p\n", (void*)request->uri_end );
   printf( "header_count:   %d\n", request->header_count );
   for( int32_t i = 0; i < request->header_count; i++ )
   {
      printf( "  [%d] %s: %s\n", i, request->headers[ i ].name, request->headers[ i ].value );
   }
   printf( "===================\n" );
}
