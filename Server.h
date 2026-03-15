#pragma once
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

#include "SocketHandler.h"

#define NUM_WORKERS 5

typedef struct
{
   SocketHandler_t socketHandler;
   int32_t         worker_kqueue_fds[ NUM_WORKERS ];
   pthread_t       server_worker_thread[ NUM_WORKERS ];
   int32_t         next_worker;
} Server_t;

void server_create( Server_t* server );

void server_destroy( Server_t* server );

// Main loop, receiving and dispatching connections
void server_start( Server_t* server );
