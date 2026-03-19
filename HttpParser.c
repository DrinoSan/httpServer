#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "HttpParser.h"
#include "Log.h"

//------------------------------------------------------------------------------
void http_parser_parse_headers( char* buffer, int32_t header_len,
                                HttpRequest_t* request )
{
   // -4 because i dont need the \r\n\r\n
   const char* headers_end = buffer + header_len - 4;

   // parsing the request line
   // If someone has a path longer than 255 then fuck that
   sscanf( buffer, "%7s %255s %15s", request->method, request->path,
           request->version );

   // Skipping the request line
   const char* line = strstr( buffer, "\r\n" ) + 2;

   int32_t idx = 0;
   while ( line < headers_end )
   {
      // name is from line to colon
      // value is from colon+2 to eol (skip ": ")
      char* colon = memchr( line, ':', headers_end - line );
      char* eol   = strstr( line, "\r\n" );

      if ( colon == NULL || colon > eol )
      {
         // Invalid header line — no colon, skip it
         line = eol + 2;
         LOG_INFO("Header line invalid!!");
         continue;
      }

      // header name handling
      int32_t name_len = colon - line;
      for ( int32_t i = 0; i < name_len; i++ )
      {
         request->headers[ idx ].name[ i ] = tolower( line[ i ] );
      }

      // We are not in C++ land with std::string need to null
      // terminate myself and i am happy to do so
      request->headers[ idx ].name[ name_len ] = '\0';

      // header value handling
      int32_t value_len = eol - ( colon + 2 );
      memcpy( request->headers[ idx ].value, colon + 2, value_len );
      request->headers[ idx ].value[ value_len ] = '\0';

      line = eol + 2;   // next line
      idx++;
   }

   request->header_count = idx + 1;
}
