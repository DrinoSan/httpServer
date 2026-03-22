#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "HttpParser.h"
#include "HttpRequest.h"
#include "Log.h"
#include "Sand_string.h"

void http_parser_sanitize_absolut_path( char* path );
bool http_parser_is_valid_version( int32_t matched, char* version );
bool http_parser_is_valid_path( int32_t matched, const char* path );
bool http_parser_parse_request_line( HttpRequest_t* request );

//------------------------------------------------------------------------------
ParseResult_t http_parser_parse_headers( Connection_t* con, int32_t header_len,
                                         HttpRequest_t* request )
{
   http_parser_parse_request_line( request );
   // -4 because i dont need the \r\n\r\n
   const char* headers_end = con->buffer + header_len - 4;

   // parsing the request line
   // If someone has a path longer than 255 then fuck that
   int32_t matched = sscanf( con->buffer, "%7s %255s %15s", request->method,
                             request->path, request->version );

   http_parser_sanitize_absolut_path( request->path );

   if ( !http_parser_is_valid_version( matched, request->version ) )
   {
      return PARSE_ERROR_MALFORMED_REQUEST_LINE;
   }

   if ( !http_parser_is_valid_path( matched, request->path ) )
   {
      return PARSE_ERROR_MALFORMED_REQUEST_LINE;
   }

   // Skipping the request line
   const char* line = strstr( con->buffer, "\r\n" ) + 2;

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

void http_parser_sanitize_absolut_path( char* path )
{
   // check for unit test test_http11_absolute_form_uri
   if ( strncmp( path, "http://", 7 ) == 0 )
   {
      char* authority = path + 7;
      char* slash     = strchr( authority, '/' );
      if ( slash == NULL )
      {
         // No path requested invalid
         path[ 0 ] = '/';
         path[ 1 ] = '\0';
      }
      else
      {
         int32_t path_len = strlen( slash );
         memmove( path, slash, path_len );
         path[ path_len ] = '\0';
      }
   }
   else if ( strncmp( path, "https://", 8 ) == 0 )
   {
      char* authority = path + 8;
      char* slash     = strchr( authority, '/' );
      if ( slash == NULL )
      {
         // No path requested invalid
         path[ 0 ] = '/';
         path[ 1 ] = '\0';
      }
      else
      {
         int32_t path_len = strlen( slash );
         memmove( path, slash, path_len );
         path[ path_len ] = '\0';
      }
   }
}

//------------------------------------------------------------------------------
bool http_parser_is_valid_version( int32_t matched, char* version )
{
   // Matched returnes how many matches it got but for %15s it would skip spaces
   // and conitnue untill the next non whitespace
   if ( matched != 3 || ( strcmp( version, "HTTP/1.1" ) != 0 &&
                          strcmp( version, "HTTP/1.0" ) != 0 ) )
   {
      return false;
   }

   return true;
}

//------------------------------------------------------------------------------
bool http_parser_is_valid_path( int32_t matched, const char* path )
{
   // * for options
   if ( matched != 3 || ( path[ 0 ] != '/' && path[ 0 ] != '*' ) )
   {
      return false;
   }

   return true;
}

//------------------------------------------------------------------------------
bool http_parser_parse_request_line( HttpRequest_t* request )
{
   // State machine ala nginx
   enum
   {
      sand_start = 0,
      sand_method,
      sand_space_befor_uri,
      sand_schema,
      sand_schema_slash,
      sand_schema_slash_slash,
      sand_spaces_before_host,
      sand_host_start,
      sand_host,
      sand_host_end,
      sand_uri,
      sand_http_09,
      sand_http_H,
      sand_http_HT,
      sand_http_HTT,
      sand_http_HTTP,
      sand_first_major_digit,
      sand_major_digit,
      sand_first_minor_digit,
      sand_minor_digit,
      sand_spaces_after_digit,
      sand_almost_done
   } state;

   // GET /index.html HTTP/1.1\r\n
   state = request->state;

   return true;
}
