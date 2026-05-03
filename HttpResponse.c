#include <stdio.h>
#include <string.h>

#include "HttpResponse.h"
#include "Log.h"
#include "atom.h"

//------------------------------------------------------------------------------
void http_response_serialize_status_line( HttpResponse_t* response,
                                          Sand_string_t*  string );

//------------------------------------------------------------------------------
const char* http_status_text( int status_code )
{
   switch ( status_code )
   {
   case 100:
      return sand_atom_string( "Continue" );
   case 200:
      return sand_atom_string( "OK" );
   case 201:
      return sand_atom_string( "Created" );
   case 204:
      return sand_atom_string( "No Content" );
   case 301:
      return sand_atom_string( "Moved Permanently" );
   case 302:
      return sand_atom_string( "Found" );
   case 400:
      return sand_atom_string( "Bad Request" );
   case 403:
      return sand_atom_string( "Forbidden" );
   case 404:
      return sand_atom_string( "Not Found" );
   case 405:
      return sand_atom_string( "Method Not Allowed" );
   case 411:
      return sand_atom_string( "Length Required" );
   case 413:
      return sand_atom_string( "Payload Too Large" );
   case 414:
      return sand_atom_string( "URI Too Long" );
   case 429:
      return sand_atom_string( "Too Many Requests" );
   case 431:
      return sand_atom_string( "Request Header Fields Too Large" );
   case 500:
      return sand_atom_string( "Internal Server Error" );
   case 501:
      return sand_atom_string( "Not Implemented" );
   case 503:
      return sand_atom_string( "Service Unavailable" );
   case 505:
      return sand_atom_string( "HTTP Version Not Supported" );
   default:
      return sand_atom_string( "Unknown" );
   }
}

//------------------------------------------------------------------------------
void http_response_set_header( HttpResponse_t* response, const char* name,
                               const char* value )
{
   int name_length = strlen( name );
   if ( name_length > MAX_HEADER_NAME_LEN )
   {
      LOG_WARN( "Header name length exceds max" );
   }

   if ( response->header_count >= MAX_HEADERS )
   {
      LOG_WARN( "MAX num of headers reached, header wil not be added" );
      return;
   }

   HttpResponseHeader_t* header = &response->headers[ response->header_count ];
   strncpy( header->name, name, MAX_HEADER_NAME_LEN - 1 );

   LOG_WARN( "HEADER NAME ADDED: %s", header->name );

   sand_string_append( &header->value, value );
   response->header_count++;
}

//------------------------------------------------------------------------------
void http_response_serialize( HttpResponse_t* response, Sand_string_t* string )
{
   http_response_serialize_status_line( response, string );

   if ( response->body == NULL )
   {
      sand_string_append( string, "Content-Length: 0\r\n\r\n" );
      return;
   }

   // Getting content-length size
   char content_length[ 64 ];
   int  body_len = strlen( response->body ) + 1;   // +1 for the "\n"

   snprintf( content_length, sizeof( content_length ), "%d", body_len );

   http_response_set_header( response, "Content-Length", content_length );

   for ( size_t i = 0; i < ( size_t ) response->header_count; i++ )
   {
      HttpResponseHeader_t* header = &response->headers[ i ];
      sand_string_append( string, header->name );
      sand_string_append( string, ": " );
      sand_string_append_n( string, header->value.data, header->value.size );
      sand_string_append( string, "\r\n" );
   }

   sand_string_append( string, "\r\n" );

   // sand_string_append( string, content_length );
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
   sand_string_append( string, http_status_text( response->status_code ) );
   sand_string_append( string, " \r\n" );
}
