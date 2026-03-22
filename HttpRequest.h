#pragma once

#include <stdint.h>

#include "HttpHeader.h"

typedef struct
{
   char         method[ 8 ];   // GET, POST, PUT, DELETE
   char         path[ 256 ];
   char         version[ 16 ];   // HTTP/1.1
   HttpHeader_t headers[ MAX_HEADERS ];
   int32_t      header_count;
   int32_t      content_length;
   char*        body;   // pointer into the connection buffer (no copy needed)
   int32_t      state;
} HttpRequest_t;

const char* http_request_find_header(const HttpRequest_t* request, const char* name);
