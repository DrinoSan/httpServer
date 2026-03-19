#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "Router.h"
#include "SocketHandler.h"

#define NUM_WORKERS 5

typedef struct Server Server_t;

typedef struct
{
   int32_t   kqueue_fd;
   Server_t* server;
} WorkerArgs_t;

typedef struct Server
{
   SocketHandler_t socketHandler;
   int32_t         worker_kqueue_fds[ NUM_WORKERS ];
   pthread_t       server_worker_thread[ NUM_WORKERS ];
   int32_t         next_worker;
   Router_t        router;
   WorkerArgs_t    worker_args[ NUM_WORKERS ];
} Server_t;

void server_create( Server_t* server );

void server_destroy( Server_t* server );

// Main loop, receiving and dispatching connections
void server_start( Server_t* server );
