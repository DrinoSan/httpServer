#pragma once

#include "sandlib/Sand_string_view.h"

#define MAX_HEADERS          32
#define MAX_HEADER_NAME_LEN  64
#define MAX_HEADER_VALUE_LEN 256

typedef struct
{
   char name[ MAX_HEADER_NAME_LEN ];
   sand_string_view_t value;
} HttpHeader_t;

