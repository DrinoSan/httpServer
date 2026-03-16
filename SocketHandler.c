#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
// Net
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "SocketHandler.h"

//------------------------------------------------------------------------------
void socketHandler_create( SocketHandler_t* socketHandler, int domain, int type,
                           int protocol )
{
   socketHandler->socketFD = socket( domain, type, protocol );
   assert( socketHandler->socketFD != -1 );

   printf( "Server required socketFD <%d>\n", socketHandler->socketFD );
}

//------------------------------------------------------------------------------
void socketHandler_destroy( SocketHandler_t* socketHandler )
{
   printf( "Destroying SocketHandler\n" );
   socketHandler_closeSocket( socketHandler );
}

//------------------------------------------------------------------------------
// Initialize the socket: bind to an address and port, then start listening.
// If the address is empty, it will bind to all interfaces.
void socketHandler_init( SocketHandler_t* socketHandler, const char* address,
                         int32_t port, int32_t backlog )
{
   // Set socket options so that we can reuse the address quickly.
   int yes = 1;
   if ( setsockopt( socketHandler->socketFD, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof( int ) ) == -1 )
   {
      perror( "Setting socketopt failed" );
      assert( false );
   }

   // Bind the socket.
   struct sockaddr_in addr;
   memset( &addr, 0, sizeof( addr ) );
   addr.sin_family = AF_INET; // IPv4
   addr.sin_port   = htons( port ); // Need to convert port to network byte order

   if ( strlen( address ) == 0 )
   {
      // If address is not set we listen to any interface
      addr.sin_addr.s_addr = INADDR_ANY;
   }
   else
   {
      // If a address is provided to use we set it up here
      if( inet_pton( AF_INET, address, &( addr.sin_addr ) ) != 1 )
      {
         perror( "Address is invalid" );
         assert( false );
      }
   }

   if ( bind( socketHandler->socketFD, ( struct sockaddr* ) &addr,
              sizeof( addr ) ) == -1 )
   {
      perror( "Binding to socket failed" );
      assert( false );
   }

   // Start listening on the socket.
   if ( listen( socketHandler->socketFD, backlog ) == -1 )
   {
      perror( "Listetning failed" );
      assert( false );
   }

   printf( "Socket bound and listening on port <%d>\n", port );
}

//------------------------------------------------------------------------------
// Accept a new connection and return the new socket file descriptor.
int32_t socketHandler_acceptConnection( SocketHandler_t* socketHandler )
{
   struct sockaddr_in clientAddr;
   socklen_t          clientLen    = sizeof( clientAddr );
   int                clientSocket = accept( socketHandler->socketFD,
                                             ( struct sockaddr* ) &clientAddr, &clientLen );
   if ( clientSocket == -1 )
   {
      perror( "Failed to accept connection" );
      return -1;
   }

   return clientSocket;
}

//------------------------------------------------------------------------------
// Close the listening socket.
void socketHandler_closeSocket( SocketHandler_t* socketHandler )
{
   if ( socketHandler->socketFD != -1 )
   {
      close( socketHandler->socketFD );
      socketHandler->socketFD = -1;
   }
}

// Reaading from socket with hardcoded timeout
// Function can throw
// int32_t socketHandler_readFromSocket( SocketHandler_t* socketHandler,
// Connection_t* conn )
//{
//   return 1231;
//}
//
// void socketHandler_writeToSocket( SocketHandler_t* Connection_t* conn, const
// char* data ) {}
