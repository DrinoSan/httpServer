#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

#define BUFFER_SIZE 8192
//------------------------------------------------------------------------------
// I need a Connection state to imrpove on my mistake from my last http server
// Basically when using kevent we dont want to have blocking calls to recv for
// example I used to have kevent and then blocking call to recv which does not
// make sens why then using kevent... I could also just spawn a thread and call
// it a day With this i can pass it via udata to EV_SET and alway identify the
// descriptior with its state
typedef enum
{
   CONN_READING_HEADERS,
   CONN_READING_BODY,
   CONN_SENDING_RESPONSE
} ConnectionState_t;

//------------------------------------------------------------------------------
typedef struct
{
   int32_t           fd;
   char              buffer[ BUFFER_SIZE ];
   int32_t           bytes_read;
   int32_t           header_len;   // offset where body starts (after \r\n\r\n)
   int32_t           content_length;
   HttpRequest_t     request;
   HttpResponse_t    response;
   ConnectionState_t state;
} Connection_t;

Connection_t* connection_create_heap( int32_t fd );
void          connection_destroy( Connection_t* con );
