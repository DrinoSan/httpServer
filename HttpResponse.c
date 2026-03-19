#include <stdio.h>
#include <string.h>

#include "HttpResponse.h"

//------------------------------------------------------------------------------
void http_response_serialize_status_line( HttpResponse_t* response,
                                          Sand_string_t*  string );

//------------------------------------------------------------------------------
void http_response_serialize( HttpResponse_t* response, Sand_string_t* string )
{
   http_response_serialize_status_line( response, string );

   if ( response->body == NULL )
   {
      sand_string_append(string, "Content-Length: 0\r\n\r\n");
      return;
   }

   // Getting content-length size
   char content_length[ 64 ];
   int  body_len = strlen( response->body ) + 1;   // +1 for the "\n"

   snprintf( content_length, sizeof( content_length ),
             "Content-Length: %d\r\n\r\n", body_len );

   sand_string_append( string, content_length );
   sand_string_append( string, response->body );
   sand_string_append( string, "\n" );
}

//------------------------------------------------------------------------------
void http_response_serialize_status_line( HttpResponse_t* response,
                                          Sand_string_t*  string )
{
   // Building status line
   sand_string_append( string, "HTTP/1.1 " );
   char status_code_str[ 20 ];
   snprintf( status_code_str, sizeof( status_code_str ), "%d",
             response->status_code );
   sand_string_append( string, status_code_str );
   sand_string_append( string, " " );
   sand_string_append( string, response->status_text );
   sand_string_append( string, " \r\n" );
}
