#pragma once

#include "HttpRequest.h"

typedef enum
{
   PARSE_OK,
   PARSE_ERROR_MALFORMED_REQUEST_LINE,
   PARSE_ERROR_INVALID_HEADERS,
   PARSE_ERROR_PATH_TOO_LONG,
} ParseResult_t;

ParseResult_t http_parser_parse_headers( char* buffer, int32_t header_len,
                                HttpRequest_t* request );
