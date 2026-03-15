#pragma once

#define MAX_HEADERS          32
#define MAX_HEADER_NAME_LEN  64
#define MAX_HEADER_VALUE_LEN 256

typedef struct
{
   char name[ MAX_HEADER_NAME_LEN ];
   char value[ MAX_HEADER_VALUE_LEN ];
} HttpHeader_t;

typedef struct
{
   char         method[ 8 ];   // GET, POST, PUT, DELETE
   char         path[ 256 ];
   char         version[ 16 ];   // HTTP/1.1
   HttpHeader_t headers[ MAX_HEADERS ];
   int32_t      header_count;
   int32_t      content_length;
   char*        body;   // pointer into the connection buffer (no copy needed)
} HttpRequest_t;

const char* http_request_find_header(const HttpRequest_t* request, const char* name);
