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
// Ownership model:
//
// Connection_t is heap-allocated (malloc) by connection_create_heap() and
// freed by connection_destroy(). Exactly one worker thread owns each
// Connection_t — no sharing between threads.
//
// Memory layout:
//
//   Connection_t  (heap, single owner: one worker thread)
//   ├── fd                 socket file descriptor, closed by connection_destroy()
//   ├── buffer[8192]       inline, dies with Connection_t
//   │   └── HttpRequest_t.uri_view      ──> string view INTO buffer (no copy)
//   │   └── HttpRequest_t.headers[].value ──> string views INTO buffer (no copy)
//   │   └── HttpRequest_t.body          ──> pointer INTO buffer (no copy)
//   ├── HttpRequest_t      inline, zero-copy views into buffer above
//   │   └── headers[].name              ──> copied + lowercased (owns the data)
//   ├── HttpResponse_t     inline
//   │   └── body                        ──> external pointer (typically string literal, NOT owned)
//   │   └── headers[].name              ──> copied by caller (owns the data)
//   │   └── headers[].value             ──> string view, caller must ensure lifetime
//   └── buf (Sand_string_t)             ──> heap data via sand_string_create()
//                                           freed by connection_destroy()
//
// Lifetime rules:
// - buffer[] and all string views into it are valid for the lifetime of Connection_t
// - response.body must point to data that outlives the connection (string literals, static buffers)
// - buf (Sand_string_t) is used for response serialization, freed on destroy
// - sand_string_destroy() sets data=NULL, so double-destroy is safe (not a crash, just redundant)
//
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
   Sand_string_t     buf;
} Connection_t;

Connection_t* connection_create_heap( int32_t fd );
void          connection_destroy( Connection_t* con );
