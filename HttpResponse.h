#pragma once
#include <stdint.h>

#include "HttpHeader.h"

typedef struct
{
   int32_t      status_code;
   char         status_text[32];
   HttpHeader_t headers[MAX_HEADERS];
   int32_t      header_count;
   char*        body;
   int32_t      body_len;
} HttpResponse_t;

