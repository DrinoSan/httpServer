#pragma once

#include "HttpRequest.h"

typedef enum
{
   PARSE_OK,
   PARSE_ERROR_MALFORMED_REQUEST_LINE,
   PARSE_ERROR_INVALID_VERSION,
   PARSE_INVALID_09_METHOD,
   PARSE_ERROR_INVALID_HEADERS,
   PARSE_ERROR_PATH_TOO_LONG,
   PARSE_ERROR_TOO_MANY_HEADERS,   // 431 Request Header
   PARSE_ERROR_MISSING_HOST,       // 400 Bad Request
   PARSE_ERROR_HEADER_NAME_TOO_LONG,
} ParseResult_t;

ParseResult_t http_parser_parse_headers( char* buffer, int32_t header_len,
                                         HttpRequest_t* request );
