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

#include <libtelnet.h>

#include "app-heap.h"
#include "app-loop.h"
#include "app-lua-service.h"
#include "app-result.h"
#include "app-service.h"


#define APP_LUA_SERVICE_MAX_CONNECTIONS             64
#define APP_LUA_SERVICE_READ_BUFFER_SIZE            4096


typedef struct {
    const app_service_session_ctx_t* ctx;
    telnet_t* telnet;
    char read_buffer[APP_LUA_SERVICE_READ_BUFFER_SIZE];
    app_event_id_t close_id;
} app_lua_service_session_t;


static void schedule_read(app_lua_service_session_t* session);


static app_service_t* m_service;


static void close_session_callback(void* user_data) {
    app_lua_service_session_t* session = user_data;

    session->close_id = 0;
    app_service_close_session(session->ctx);
}


static void schedule_close(app_lua_service_session_t* session) {
    // defer session close until next app_event_poll
    if ( session->close_id == 0 ) {
        session->close_id = app_event_register_timer(0, close_session_callback, session);
    }
}


static void handle_telnet_recv_event(app_lua_service_session_t* session, telnet_event_t* event) {

}


static void handle_telnet_send_event(app_lua_service_session_t* session, telnet_event_t* event) {
    const char* buffer_i = event->data.buffer;
    const char* buffer_e = buffer_i + event->data.size;

    while(buffer_i < buffer_e) {
        ssize_t n_written;

        n_written = app_stream_write_sync(session->ctx->stream, buffer_i, buffer_e - buffer_i);

        if ( n_written < 0 ) {
            // TODO: Log write failure
            schedule_close(session);
            return;
        }

        buffer_i += n_written;
    }
}


static void telnet_callback(telnet_t* telnet, telnet_event_t* event, void* user_data) {
    app_lua_service_session_t* session = user_data;

    switch(event->type) {
    case TELNET_EV_DATA:
        handle_telnet_recv_event(session, event);
        break;

    case TELNET_EV_SEND:
        handle_telnet_send_event(session, event);
        break;

    case TELNET_EV_ERROR:

        break;

    default:
        break;
    }
}


static void read_callback(app_stream_t* stream, app_result_t result, ssize_t n_transferred, void* user_data) {
    app_lua_service_session_t* session = user_data;

    if ( result == APP_RESULT_OK ) {
        // feed telnet context with received data
        telnet_recv(session->telnet, session->read_buffer, n_transferred);
    }

    // continue receiving input
    schedule_read(session);
}


static void schedule_read(app_lua_service_session_t* session) {
    app_stream_read(session->ctx->stream, session->read_buffer, APP_LUA_SERVICE_READ_BUFFER_SIZE,
            read_callback, session, NULL);
}


static void* session_create_callback(app_service_t* service, const app_service_session_ctx_t* ctx, void* user_data) {
    static const telnet_telopt_t TELNET_OPTS[] = {
        {TELNET_TELOPT_ECHO,        TELNET_WILL,    TELNET_DONT},   // I will echo, you don't need to
        {TELNET_TELOPT_BINARY,      TELNET_WILL,    TELNET_DO},     // I will use binary mode, you should too
        {TELNET_TELOPT_NAWS,        TELNET_WONT,    TELNET_DO},     // I won't send window size changes, but you should
        {TELNET_TELOPT_LINEMODE,    TELNET_WONT,    TELNET_DONT},   // I won't use line mode, neither should you
        {TELNET_TELOPT_SGA,         TELNET_WONT,    TELNET_DONT},   // Enable 'suppress go-ahead'
        {-1, 0, 0} // end marker
    };

    app_lua_service_session_t* session;

    // unused arguments
    (void) service;
    (void) user_data;

    // create session object
    session = app_heap_alloc(sizeof(app_lua_service_session_t));

    session->ctx = ctx;
    session->telnet = telnet_init(TELNET_OPTS, telnet_callback, 0, session);
    session->close_id = 0;

    // wait for input
    schedule_read(session);

    return session;
}


static void session_destroy_callback(void* session_object, void* user_data) {
    app_lua_service_session_t* session = session_object;

    // unused arguments
    (void) user_data;

    // cancel any close events
    app_event_unregister_timer(session->close_id);
    session->close_id = 0;

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
