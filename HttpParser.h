#pragma once

#include "HttpRequest.h"

void http_parser_parse_headers( char* buffer, int32_t header_len, HttpRequest_t* request );
