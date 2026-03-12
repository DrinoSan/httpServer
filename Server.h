#pragma once
#include <stdbool.h>



#include "SocketHandler.h"

typedef struct
{
  SocketHandler_t socketHandler;
} Server_t;

void server_create( Server_t* server );

void server_destroy( Server_t* server );

