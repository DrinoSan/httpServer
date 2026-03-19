#pragma once
#include <stdint.h>

#include "HttpHeader.h"
#include "Sand_string.h"

typedef struct
{
   int32_t      status_code;
   char         status_text[32];
   HttpHeader_t headers[MAX_HEADERS];
   int32_t      header_count;
   char*        body;
   int32_t      body_len;
} HttpResponse_t;

void http_response_serialize( HttpResponse_t* response, Sand_string_t* string );
