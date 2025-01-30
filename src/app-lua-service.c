/*
 * x0 - A lightweight RISC-V (RV32IM) simulator with GDB and Lua integration.
 *
 * Copyright (C) 2024-2025 Matthew T. Bucknall <matthew.bucknall@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>

#include "app-heap.h"
#include "app-loop.h"
#include "app-lua-service.h"
#include "app-result.h"
#include "app-service.h"


#define APP_LUA_SERVICE_MAX_CONNECTIONS             64


typedef struct {
    app_stream_t* stream;
} app_lua_service_session_t;


static app_service_t* m_service;


static void* session_create_callback(app_service_t* service, app_stream_t* stream, void* user_data) {
    app_lua_service_session_t* session;

    // unused arguments
    (void) service;
    (void) user_data;

    // create session object
    session = app_heap_alloc(sizeof(app_lua_service_session_t));

    session->stream = stream;

    return session;
}


static void session_destroy_callback(void* session_object, void* user_data) {
    // unused arguments
    (void) user_data;

    // destroy session object
    app_heap_free(session_object);
}


static void cleanup(void) {
    // destroy service and any active sessions
    app_service_destroy(m_service);
    m_service = NULL;
}


void app_lua_service_init(const struct sockaddr_in* addr) {
    // register cleanup function
    if ( atexit(cleanup) < 0 ) {
        abort(); // no return
    }

    // create service
    m_service = app_service_new("lua", addr, APP_LUA_SERVICE_MAX_CONNECTIONS,
            session_create_callback, session_destroy_callback, NULL);

    if ( m_service == NULL ) {
        app_loop_stop(APP_RESULT_CANNOT_BIND_SERVICE);
    }
}
