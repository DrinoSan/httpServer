#pragma once

// System Headers
#include <netdb.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
   int socketFD;
}
SocketHandler_t;

void socketHandler_create( SocketHandler_t* socketHandler, int domain, int type, int protocol );

void socketHandler_destroy( SocketHandler_t* socketHandler );

// Initialize the socket: bind to an address and port, then start listening.
// If the address is empty, it will bind to all interfaces.
void socketHandler_init( SocketHandler_t* socketHandler, const char* address,
                         int32_t port, int32_t backlog );

// Accept a new connection and return the new socket file descriptor.
int32_t socketHandler_acceptConnection( SocketHandler_t* socketHandler );

// Close the listening socket.
void socketHandler_closeSocket( SocketHandler_t* socketHandler );

// Reaading from socket with hardcoded timeout
// Function can throw
//int32_t socketHandler_readFromSocket( SocketHandler_t* socketHandler,
//                                      Connection_t*    conn );
//
//void socketHandler_writeToSocket( SocketHandler_t* Connection_t* conn,
//                                  const char*                    data );

// NOT USED //
// Function to check if there is any data to read on socketFD
//[[nodiscard]] static bool hasIncomingData( int socketFD );
