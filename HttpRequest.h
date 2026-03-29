#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "HttpHeader.h"
#include "sandlib/Sand_string_view.h"

// clang-format off
// Using power of twos because that way only one single bit is set in the mask
// which i need for bit operations
//
// Each HEX digit is 4 bit digits            POWER OF
#define SAND_HTTP_UNKNOWN     0x00000001   // 0
#define SAND_HTTP_GET         0x00000002   // 1
#define SAND_HTTP_HEAD        0x00000004   // 2
#define SAND_HTTP_POST        0x00000008   // 3
#define SAND_HTTP_PUT         0x00000010   // 4
#define SAND_HTTP_DELETE      0x00000020   // 5
#define SAND_HTTP_MKCOL       0x00000040   // 6
#define SAND_HTTP_COPY        0x00000080   // 7
#define SAND_HTTP_MOVE        0x00000100   // 8
#define SAND_HTTP_OPTIONS     0x00000200   // 9
#define SAND_HTTP_PROPFIND    0x00000400   // 10
#define SAND_HTTP_PROPPATCH   0x00000800   // 11
#define SAND_HTTP_LOCK        0x00001000   // 12
#define SAND_HTTP_UNLOCK      0x00002000   // 13
#define SAND_HTTP_PATCH       0x00004000   // 14
#define SAND_HTTP_TRACE       0x00008000   // 15
#define SAND_HTTP_CONNECT     0x00010000   // 16
#define SAND_HTTP_ALL_METHODS 0x0001FFFF   // If we double SAND_HTTP_CONNECT by 2 then subtract one we get the ALL_Methos
// By Doubeling we shift the one bit to the left (Division would shift right) Then if we subtract one if fills all the bits after with one
// clang-format on

typedef struct
{
   char         method[ 8 ];   // GET, POST, PUT, DELETE
   int32_t      method_int;
   char         path[ 256 ];
   char         version[ 16 ];   // HTTP/1.1
   int32_t      version_int;     // For easier checking
   HttpHeader_t headers[ MAX_HEADERS ];
   int32_t      header_count;
   int32_t      content_length;
   bool         ignore_content_length;   // Should be set if content-length and
                                         // transfer_encoding is set
   char* body;   // pointer into the connection buffer (no copy needed)

   int32_t state;
   char*   request_start;
   char*   request_end;

   char* schema_start;
   char* schema_end;

   char* method_end;

   char*              uri_start;
   char*              uri_end;
   sand_string_view_t uri_view;

   char* host_start;
   char* host_end;

   int32_t http_major;
   int32_t http_minor;
   int32_t complex_uri;

} HttpRequest_t;

const sand_string_view_t*
http_request_find_header( const HttpRequest_t* request, const char* name );

void http_request_showMe( const HttpRequest_t* request );
