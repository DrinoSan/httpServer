#pragma once
#include <stdint.h>

#include "HttpHeader.h"
#include "Sand_string.h"

typedef struct
{
   int32_t              status_code;
   char                 status_text[ 32 ];
   HttpResponseHeader_t headers[ MAX_HEADERS ];
   int32_t              header_count;
   char*                body;
   int32_t              body_len;
} HttpResponse_t;

// -----------------------------------------------------------------------------
// Function to serialize the response and prepare for sending
// @param response which will be sent via send
// @param string holding the response body which will be serialized
void http_response_serialize( HttpResponse_t* response, Sand_string_t* string );

// -----------------------------------------------------------------------------
/// Function to append a header to the response headers
/// @param response which will be headers appended to
/// @param name which is the name of th header
/// @param value which is the value of the heade
void http_response_set_header( HttpResponse_t* response, const char* name, const char* value );
