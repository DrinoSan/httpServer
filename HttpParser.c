#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "HttpParser.h"
#include "HttpRequest.h"
#include "Log.h"
#include "Sand_string.h"

//------------------------------------------------------------------------------
ParseResult_t http_parser_parse_headers( char* buffer, int32_t header_len,
                                         HttpRequest_t* request )
{
   // -4 because i dont need the \r\n\r\n
   const char* headers_end = buffer + header_len - 4;

   // parsing the request line
   // If someone has a path longer than 255 then fuck that
   sscanf( buffer, "%7s %255s %15s", request->method, request->path,
           request->version );

   // check for unit test test_http11_absolute_form_uri
   if ( strncmp( request->path, "http://", 7 ) == 0 )
   {
      char* authority = request->path + 7;
      char* slash     = strchr( authority, '/' );
      if ( slash == NULL )
      {
         // No path requested invalid
         request->path[ 0 ] = '/';
         request->path[ 1 ] = '\0';
      }
      else
      {
         int32_t path_len = strlen( slash );
         memmove( request->path, slash, path_len );
         request->path[ path_len ] = '\0';
      }
   }
   else if ( strncmp( request->path, "https://", 8 ) == 0 )
   {
      char* authority = request->path + 8;
      char* slash     = strchr( authority, '/' );
      if ( slash == NULL )
      {
         // No path requested invalid
         request->path[ 0 ] = '/';
         request->path[ 1 ] = '\0';
      }
      else
      {
         int32_t path_len = strlen( slash );
         memmove( request->path, slash, path_len );
         request->path[ path_len ] = '\0';
      }
   }

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
         LOG_INFO( "Header line invalid!!" );
         return PARSE_ERROR_INVALID_HEADERS;
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

      // Trimming whitespaces
      sand_string_trim_cstr( request->headers[ idx ].name, 0 );

      // header value handling
      int32_t value_len = eol - ( colon + 2 );
      memcpy( request->headers[ idx ].value, colon + 2, value_len );
      request->headers[ idx ].value[ value_len ] = '\0';

      // Trimming whitespaces
      sand_string_trim_cstr( request->headers[ idx ].value, 0 );

      line = eol + 2;   // next line
      idx++;
      request->header_count += 1;

      if ( request->header_count >= MAX_HEADERS )
      {
         return PARSE_ERROR_TOO_MANY_HEADERS;
      }
   }

   // Further checks on headers
   if ( strcmp( request->version, "HTTP/1.1" ) == 0 )
   {
      if ( http_request_find_header( request, "host" ) == NULL )
      {
         // Bad Request
         return PARSE_ERROR_MISSING_HOST;
      }
   }

   return PARSE_OK;
}
