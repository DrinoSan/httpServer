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
