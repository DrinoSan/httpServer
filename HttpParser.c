#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "HttpParser.h"
#include "HttpRequest.h"
#include "Log.h"
#include "Sand_string.h"

#define CR '\r'
#define LF '\n'

// clang-format off
// I assume little endian
//  Address:  0      1      2      3
//  Value:    0x47   0x45   0x54   0x20
//            'G'    'E'    'T'    ' '

//  Address 0 → bits  0-7   (0x47)  = 0x00000047
//  Address 1 → bits  8-15  (0x45)  = 0x00004500
//  Address 2 → bits 16-23  (0x54)  = 0x00540000
//  Address 3 → bits 24-31  (0x20)  = 0x20000000
//                                    ----------
//  Result:                           0x20544547
// clang-format on

// Assuming "GET "
// Thats why c3 (the space) has to be moved 24 bis to the left because in M it
// stands like this " TEG"
#define sand_str3_cmp( m, c0, c1, c2, c3 )                                     \
   *( uint32_t* ) m == ( ( c3 << 24 ) | ( c2 << 16 ) | ( c1 << 8 ) | c0 )

#define sand_str4_cmp( m, c0, c1, c2, c3 )                                     \
   *( uint32_t* ) m == ( ( c3 << 24 ) | ( c2 << 16 ) | ( c1 << 8 ) | c0 )

// clang-format off
// First we cast it to a ( uin32_t* )m
// Second the [1] — index 1 means "we start at the second uint32_t", which // starts at byte 4
// Third we 0xffff the 2 most significant bytes which is garbage in little endian our 'T' 'E' is in the least significant bytes
// clang-format on
#define sand_str6_cmp( m, c0, c1, c2, c3, c4, c5 )                             \
   *( uint32_t* ) m == ( ( c3 << 24 ) | ( c2 << 16 ) | ( c1 << 8 ) | c0 ) &&   \
       ( ( ( uint32_t* ) m )[ 1 ] & 0xffff ) == ( ( c5 << 8 ) | c4 )

void http_parser_sanitize_absolut_path( char* path );
bool http_parser_is_valid_version( int32_t version );
bool http_parser_is_valid_path( const char* path );
bool http_parser_parse_request_line( HttpRequest_t* request, char* start,
                                     char* end );

// NGINX state machine taken as guideline
bool http_parser_parse_request_line( HttpRequest_t* request, char* start,
                                     char* end )
{
   request->method_int = SAND_HTTP_UNKNOWN;

   enum
   {
      sand_start = 0,
      sand_method,
      sand_space_befor_uri,
      sand_schema,
      sand_schema_slash,
      sand_schema_slash_slash,
      sand_after_slash_in_uri,
      sand_check_uri,
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
      sand_almost_done,

      sand_host_start
   } state;

   char* pos = start;
   char* method;
   char  c;

   state = sand_start;

   while ( pos != end )
   {
      char ch = *pos;
      switch ( state )
      {
      case sand_start:
      {
         request->request_start = pos;

         // HTTP/1.1 spec (RFC 7230, Section 3.5)
         // Skipping empty lines before real request line
         if ( ch == CR || ch == LF )
         {
            break;
         }

         if ( ( ch < 'A' || ch > 'Z' ) && ch != '_' && ch != '-' )
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         state = sand_method;
         break;
      }

      case sand_method:
      {
         if ( ch == ' ' )
         {
            request->method_end = pos - 1;
            method              = request->request_start;

            state = sand_space_befor_uri;

            switch ( pos - method )
            {
            case 3:
            {
               if ( sand_str3_cmp( method, 'G', 'E', 'T', ' ' ) )
               {
                  request->method_int = SAND_HTTP_GET;
                  break;
               }

               if ( sand_str3_cmp( method, 'P', 'U', 'T', ' ' ) )
               {
                  request->method_int = SAND_HTTP_PUT;
                  break;
               }

               break;
            }

            case 4:
            {
               if ( sand_str4_cmp( method, 'P', 'O', 'S', 'T' ) )
               {
                  request->method_int = SAND_HTTP_POST;
                  break;
               }

               break;
            }

            case 6:
            {
               if ( sand_str6_cmp( method, 'D', 'E', 'L', 'E', 'T', 'E' ) )
               {
                  request->method_int = SAND_HTTP_DELETE;
                  break;
               }

               break;
            }

            break;
            }
         }
         else
         {
            if ( ( ch < 'A' || ch > 'Z' ) && ch != '_' && ch != '-' )
            {
               return PARSE_ERROR_MALFORMED_REQUEST_LINE;
            }

            break;
         }
      }

      case sand_space_befor_uri:
      {
         if ( ch == '/' )
         {
            request->uri_start = pos;
            state              = sand_after_slash_in_uri;
            break;
         }

         // Lowercase and upper case letter differ by one bit at index 5
         c = ( unsigned char ) ( ch | 0x20 );
         if ( c >= 'a' && c <= 'z' )
         {
            request->schema_start = pos;
            state                 = sand_schema;
            break;
         }

         break;
      }

      case sand_after_slash_in_uri:
      {
         switch ( ch )
         {
         case ' ':
         {
            request->uri_end = pos;
            // Creating the string
            request->uri_view.data = request->uri_start;
            request->uri_view.size = request->uri_end - request->uri_start;

            state = sand_http_09;
            break;
         }
         case CR:
         {
            request->uri_end = pos;
            state            = sand_almost_done;
            break;
         }
         case LF:
         {
            request->uri_end    = pos;
            request->http_minor = 9;
            goto done;
            break;
         }
         case '/':
         {
            request->complex_uri = 1;
            state                = sand_uri;
            break;
         }
         case '.':
         {
            request->complex_uri = 1;
            state                = sand_uri;
            break;
         }
         default:
         {
            // Checking if its a printable char
            // 0x7f is DEL
            // man ascii
            if ( ch < 0x20 || ch == 0x7f )
            {
               return PARSE_ERROR_MALFORMED_REQUEST_LINE;
            }

            state = sand_uri;
            break;
         }
         }

         break;
      }

      case sand_schema:
      {
         // Lowercase and upper case letter differ by one bit at index 5
         c = ( unsigned char ) ( ch | 0x20 );
         if ( c >= 'a' && c <= 'z' )
         {
            break;
         }

         if ( ( ch >= '0' && ch <= '9' ) || ch == '+' || ch == '-' ||
              ch == '.' )
         {
            break;
         }

         switch ( ch )
         {
         case ':':
         {
            request->schema_end = pos;
            state               = sand_schema_slash;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }

         // GET /index.html HTTP/1.1\r\n

         break;
      }

      case sand_schema_slash:
      {
         switch ( ch )
         {
         case '/':
         {
            state = sand_schema_slash_slash;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }
         break;
      }

      case sand_schema_slash_slash:
      {
         switch ( ch )
         {
         case '/':
         {
            state = sand_host_start;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }
         break;
      }

      case sand_http_09:
      {
         switch ( ch )
         {
         case ' ':
         {
            break;
         }
         case CR:
         {
            request->http_minor = 9;
            state               = sand_almost_done;
            break;
         }
         case LF:
         {
            request->http_minor = 9;
            goto done;
         }
         case 'H':
         {
            // request->http_protocol.data = pos;
            state = sand_http_H;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         break;
         }
         break;
      }

      case sand_http_H:
      {
         switch ( ch )
         {
         case 'T':
         {
            state = sand_http_HT;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }

         break;
      }
      case sand_http_HT:
      {
         switch ( ch )
         {
         case 'T':
         {
            state = sand_http_HTT;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }

         break;
      }
      case sand_http_HTT:
      {
         switch ( ch )
         {
         case 'P':
         {
            state = sand_http_HTTP;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }

         break;
      }
      case sand_http_HTTP:
      {
         switch ( ch )
         {
         case '/':
         {
            state = sand_first_major_digit;
            break;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }

         break;
      }

      case sand_first_major_digit:
      {
         if ( ch < '1' || ch > '9' )
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         request->http_major = ch - '0';

         state = sand_major_digit;
         break;
      }

      case sand_major_digit:
      {
         if ( ch == '.' )
         {
            state = sand_first_minor_digit;
            break;
         }

         // If its not a number we know its a malformed request line
         if ( ch < '0' || ch > '9' )
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         // Without this line i cant check the full number like HTTP/12.0
         // in this stage the 1 is already stored in http_major, ch can only
         // be a number because the checks above
         request->http_major = request->http_major * 10 + ( ch - '0' );

         if ( request->http_major > 1 )
         {
            return PARSE_ERROR_INVALID_VERSION;
         }

         break;
      }

      case sand_first_minor_digit:
      {
         if ( ch < '0' || ch > '9' )
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         request->http_minor = ch - '0';

         state = sand_minor_digit;
         break;
      }

      case sand_minor_digit:
      {
         if ( ch == CR )
         {
            state = sand_almost_done;
            break;
         }

         // If its not a number we know its a malformed request line
         if ( ch < '0' || ch > '9' )
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         if ( ch == LF )
         {
            goto done;
         }

         if ( ch == ' ' )
         {
            state = sand_spaces_after_digit;
            break;
         }

         if ( ch < '0' || ch > '9' )
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         // Max accepting minor up to 99 because we accumulate every digit
         // example HTTP/1.123
         if ( request->http_minor > 99 )
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }

         request->http_minor = request->http_minor * 10 + ( ch - '0' );
         break;
      }

      case sand_spaces_after_digit:
      {
         switch ( ch )
         {
         case ' ':
         {
            break;
         }
         case CR:
         {
            state = sand_almost_done;
            break;
         }
         case LF:
         {
            goto done;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }
         break;
      }

      case sand_almost_done:
      {
         request->request_end = pos - 1;

         switch ( ch )
         {
         case LF:
         {
            goto done;
         }
         default:
         {
            return PARSE_ERROR_MALFORMED_REQUEST_LINE;
         }
         }
      }

      case sand_uri:
      {
         switch ( ch )
         {
         case ' ':
         {
            request->uri_end = pos;
            state            = sand_http_09;

            // Creating the string
            request->uri_view.data = request->uri_start;
            request->uri_view.size = request->uri_end - request->uri_start;
            break;
         }
         case CR:
         {
            request->uri_end = pos;
            state            = sand_almost_done;
            break;
         }
         case LF:
         {
            request->uri_end    = pos;
            request->http_minor = 9;
            goto done;
            break;
         }
         case '/':
         {
            request->complex_uri = 1;
            state                = sand_uri;
            break;
         }
         case '.':
         {
            request->complex_uri = 1;
            state                = sand_uri;
            break;
         }
         default:
         {
            // Checking if its a printable char
            // 0x7f is DEL
            // man ascii
            if ( ch < 0x20 || ch == 0x7f )
            {
               return PARSE_ERROR_MALFORMED_REQUEST_LINE;
            }

            state = sand_uri;
            break;
         }
         }
         break;
      }

      case sand_check_uri:
      {
         LOG_WARN( "Entered sand_check_uri" );
         assert( false );
         break;
      }

      case sand_host_start:
      {
         LOG_WARN( "Entered sand_host_start" );
         assert( false );
         break;
      }

      default:
      {
         LOG_WARN( "Entered most outter default state" );
         assert( false );
         break;
      }
      }

      pos++;
   }

done:

   if ( request->request_end == NULL )
   {
      request->request_end = pos;
   }

   request->version_int = request->http_major * 1000 + request->http_minor;
   request->state       = sand_start;

   if ( request->version_int == 9 && request->method_int != SAND_HTTP_GET )
   {
      return PARSE_INVALID_09_METHOD;
   }

   return true;
}

//------------------------------------------------------------------------------
ParseResult_t http_parser_parse_headers( char* buffer, int32_t header_len,
                                         HttpRequest_t* request )
{
   // -4 because i dont need the \r\n\r\n
   const char* headers_end        = buffer + header_len - 4;
   char*       request_line_start = buffer;
   char*       request_line_end   = buffer;
   while ( *request_line_end != CR )
   {
      request_line_end++;
   }

   // parsing the request line
   // If someone has a path longer than 255 then fuck that
   //   int32_t matched = sscanf( buffer, "%7s %255s %15s", request->method,
   //                             request->path, request->version );
   //
   //   http_parser_sanitize_absolut_path( request->path );
   http_parser_parse_request_line( request, buffer, headers_end );

   if ( !http_parser_is_valid_version( request->version_int ) )
   {
      return PARSE_ERROR_MALFORMED_REQUEST_LINE;
   }

   if ( !http_parser_is_valid_path( request->uri_start ) )
   {
      return PARSE_ERROR_MALFORMED_REQUEST_LINE;
   }

   // Skipping the request line
   const char* line = strstr( buffer, "\r\n" ) + 2;

   int32_t idx = 0;
   while ( line < headers_end )
   {
      // name is from line to colon
      // value is from colon+2 to eol (skip ": ")
      char* colon = memchr( line, ':', headers_end - line );
      if( colon > line && colon < headers_end )
      {
         char* check_space = colon;
         check_space--;

         // RFC 7230 Section 3.2.4 space in front of ':' is forbidden
         if( *check_space == ' ' )
         {
            return PARSE_ERROR_INVALID_HEADERS;
         }
      }

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
   if ( request->version_int == 1001 )
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
bool http_parser_is_valid_version( int32_t version )
{
   if ( version < 1000 || version > 1001 )
   {
      return false;
   }

   return true;
}

//------------------------------------------------------------------------------
bool http_parser_is_valid_path( const char* path )
{
   // * for options
   if ( path[ 0 ] != '/' && path[ 0 ] != '*' )
   {
      return false;
   }

   return true;
}
