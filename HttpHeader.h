#pragma once

#include "sandlib/Sand_string.h"
#include "sandlib/Sand_string_view.h"

#define MAX_HEADERS          32
#define MAX_HEADER_NAME_LEN  64
#define MAX_HEADER_VALUE_LEN 256

// Zero-copy view into connection buffer. No allocation, no cleanup needed.
// Lifetime: valid as long as the Connection_t is alive.
typedef struct
{
   char               name[ MAX_HEADER_NAME_LEN ];
   sand_string_view_t value;
} HttpHeader_t;

// Owned value. Handler can append any data. Freed on connection destroy.
typedef struct
{
   char          name[ MAX_HEADER_NAME_LEN ];
   Sand_string_t value;
} HttpResponseHeader_t;
