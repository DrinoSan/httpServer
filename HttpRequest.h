#pragma once

#include <stdint.h>

#include "HttpHeader.h"

#define SAND_HTTP_UNKNOWN   0x00000001
#define SAND_HTTP_GET       0x00000002
#define SAND_HTTP_HEAD      0x00000004
#define SAND_HTTP_POST      0x00000008
#define SAND_HTTP_PUT       0x00000010
#define SAND_HTTP_DELETE    0x00000020
#define SAND_HTTP_MKCOL     0x00000040
#define SAND_HTTP_COPY      0x00000080
#define SAND_HTTP_MOVE      0x00000100
#define SAND_HTTP_OPTIONS   0x00000200
#define SAND_HTTP_PROPFIND  0x00000400
#define SAND_HTTP_PROPPATCH 0x00000800
#define SAND_HTTP_LOCK      0x00001000
#define SAND_HTTP_UNLOCK    0x00002000
#define SAND_HTTP_PATCH     0x00004000
#define SAND_HTTP_TRACE     0x00008000
#define SAND_HTTP_CONNECT   0x00010000

typedef struct
{
   char         method[ 8 ];   // GET, POST, PUT, DELETE
   int32_t      method_int;
   char         path[ 256 ];
   char         version[ 16 ];   // HTTP/1.1
   HttpHeader_t headers[ MAX_HEADERS ];
   int32_t      header_count;
   int32_t      content_length;
   char*        body;   // pointer into the connection buffer (no copy needed)

   char* request_start;
   char* method_end;
   char* uri_start;
} HttpRequest_t;

const char* http_request_find_header( const HttpRequest_t* request,
                                      const char*          name );
