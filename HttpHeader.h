#pragma once

#define MAX_HEADERS          32
#define MAX_HEADER_NAME_LEN  64
#define MAX_HEADER_VALUE_LEN 256

typedef struct
{
   char name[ MAX_HEADER_NAME_LEN ];
   char value[ MAX_HEADER_VALUE_LEN ];
} HttpHeader_t;

