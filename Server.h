#pragma once
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "Router.h"
#include "SocketHandler.h"

#define NUM_WORKERS 5

//------------------------------------------------------------------------------
typedef struct Server Server_t;

//------------------------------------------------------------------------------
typedef struct
{
   int32_t   kqueue_fd;
   Server_t* server;
} WorkerArgs_t;

//------------------------------------------------------------------------------
typedef struct Server
{
   SocketHandler_t socketHandler;
   int32_t         worker_kqueue_fds[ NUM_WORKERS ];
   pthread_t       server_worker_thread[ NUM_WORKERS ];
   int32_t         next_worker;
   Router_t        router;
   WorkerArgs_t    worker_args[ NUM_WORKERS ];
} Server_t;

//------------------------------------------------------------------------------
void server_create( Server_t* server );

//------------------------------------------------------------------------------
void server_destroy( Server_t* server );

//------------------------------------------------------------------------------
// Main loop, receiving and dispatching connections
void server_start( Server_t* server );

//------------------------------------------------------------------------------
void server_serve_static_files( Server_t* server, const char* file_path,
                                const char* endpoint );

//------------------------------------------------------------------------------
void server_request_keep_alive_check( Connection_t* con );

//------------------------------------------------------------------------------
/// Function to set default headers for response
/// Currently content-length and keep alive are set
void http_server_set_default_headers_for_response( Connection_t* con );

// For setting content type
static const struct
{
   const char* ext;
   size_t      len;
   const char* mime;
} mime_types[] = {
    // Text
    { "html", 4, "text/html; charset=utf-8" },
    { "css", 3, "text/css; charset=utf-8" },
    { "csv", 3, "text/csv" },
    { "txt", 3, "text/plain; charset=utf-8" },
    { "xml", 3, "application/xml" },

    // JavaScript / JSON
    { "js", 2, "text/javascript; charset=utf-8" },
    { "json", 4, "application/json" },
    { "wasm", 4, "application/wasm" },

    // Images
    { "png", 3, "image/png" },
    { "jpg", 3, "image/jpeg" },
    { "jpeg", 4, "image/jpeg" },
    { "gif", 3, "image/gif" },
    { "webp", 4, "image/webp" },
    { "svg", 3, "image/svg+xml" },
    { "ico", 3, "image/x-icon" },
    { "avif", 4, "image/avif" },

    // Fonts
    { "woff", 4, "font/woff" },
    { "woff2", 5, "font/woff2" },
    { "ttf", 3, "font/ttf" },
    { "otf", 3, "font/otf" },

    // Audio / Video
    { "mp3", 3, "audio/mpeg" },
    { "ogg", 3, "audio/ogg" },
    { "mp4", 3, "video/mp4" },
    { "webm", 4, "video/webm" },

    // Documents
    { "pdf", 3, "application/pdf" },
    { "zip", 3, "application/zip" },
};
