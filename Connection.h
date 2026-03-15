#pragma once

//------------------------------------------------------------------------------
// I need a Connection state to imrpove on my mistake from my last http server
// Basically when using kevent we dont want to have blocking calls to recv for example
// I used to have kevent and then blocking call to recv which does not make sens why then using kevent...
// I could also just spawn a thread and call it a day
// With this i can pass it via udata to EV_SET and alway identify the descriptior with its state
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
   char              buffer[ 4096 ];
   int32_t           bytes_read;
   ConnectionState_t state;
} Connection_t;
