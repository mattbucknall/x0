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

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "app-assert.h"
#include "app-event.h"
#include "app-heap.h"
#include "app-log.h"
#include "app-service.h"


typedef struct app_service_session {
    int session_socket;
    app_stream_t* stream;
    void* session_object;
} app_service_session_t;


struct app_service {
    char* name;
    unsigned int max_connections;
    app_service_create_session_callback_t create_session_cb;
    app_service_destroy_session_callback_t destroy_session_cb;
    void* user_data;
    unsigned int session_count;
    unsigned int session_capacity;
    app_service_session_t* sessions;
    int listen_socket;
    app_event_id_t listen_id;
};


static void schedule_accept(app_service_t* service);


static void close_socket(int skt) {
    int close_result;

    if ( skt >= 0 ) {
        do {
            close_result = close(skt);
        } while(close_result < 0 && errno == EINTR);
    }
}


static void accept_callback(uint32_t events, void* user_data) {
    app_service_t* service = user_data;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    // handle connection event
    if ( events & APP_EVENT_IN ) {
        app_log_info("%s service accepting connection", service->name);

        close_socket(accept(service->listen_socket, (struct sockaddr*) &client_addr, &client_addr_len));

        // TODO: Create session
    }

    // wait for next connection event
    service->listen_id = 0;
    schedule_accept(service);
}


static void schedule_accept(app_service_t* service) {
    APP_ASSERT(service);
    APP_ASSERT(service->listen_id == 0);

    // wait for connection event
    service->listen_id = app_event_register_io(service->listen_socket, APP_EVENT_IN, accept_callback, service);
}


static void cleanup_session(app_service_t* service, app_service_session_t* session) {
    service->destroy_session_cb(session->session_object, service->user_data);
    app_stream_destroy(session->stream);
    close_socket(session->session_socket);
}


app_service_t* app_service_new(const char* name, const struct sockaddr_in* addr, unsigned int max_conns,
        app_service_create_session_callback_t create_session_cb,
        app_service_destroy_session_callback_t destroy_session_cb,
        void* user_data) {
    APP_ASSERT(addr);
    APP_ASSERT(max_conns > 0);
    APP_ASSERT(create_session_cb);
    APP_ASSERT(destroy_session_cb);

    app_service_t* service;
    char addr_str[INET_ADDRSTRLEN];
    int listen_socket;

    // validate listen address and convert to string
    if (inet_ntop(AF_INET, &(addr->sin_addr), addr_str, sizeof(addr_str)) == NULL) {
        app_log_error("Cannot start %s service: Invalid bind address", name);
        return NULL;
    }

    // create server socket
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    if ( listen_socket >= 0 ) {
        int opt = 1;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    if ( listen_socket < 0 || bind(listen_socket, (struct sockaddr*) addr, sizeof(*addr)) < 0 ||
            listen(listen_socket, 8) < 0 ) {
        app_log_error("%s service unable to bind to %s:%u: %s", name, addr_str, ntohs(addr->sin_port),
                strerror(errno));

        close_socket(listen_socket);
        return NULL;
    }

    // create service object
    service = app_heap_alloc(sizeof(app_service_t));

    service->name = app_heap_strdup(name);
    service->max_connections = max_conns;
    service->create_session_cb = create_session_cb;
    service->destroy_session_cb = destroy_session_cb;
    service->user_data = user_data;
    service->session_count = 0;
    service->session_capacity = 16;
    service->sessions = app_heap_alloc(sizeof(app_service_session_t) * service->session_capacity);
    service->listen_socket = listen_socket;
    service->listen_id = 0;

    // log start message
    app_log_info("%s service listening on %s:%u", service->name, addr_str, ntohs(addr->sin_port));

    // start accepting connections
    schedule_accept(service);

    return service;
}


void app_service_destroy(app_service_t* service) {
    if ( service ) {
        // log stop message
        app_log_info("Stopping %s service", service->name);

        // destroy all sessions
        for (size_t i = 0; i < service->session_count; ++i) {
            cleanup_session(service, &service->sessions[i]);
        }

        app_heap_free(service->sessions);
        service->session_count = 0;
        service->session_capacity = 0;
        service->sessions = NULL;

        // cancel any listen events
        app_event_unregister_io(service->listen_id);
        service->listen_id = 0;

        // close listening socket
        close_socket(service->listen_socket);
        service->listen_socket = -1;

        // free name
        app_heap_free(service->name);
        service->name = NULL;

        // destroy service object
        app_heap_free(service);
    }
}


void app_service_destroy_session(app_service_t* service, void* session_object) {
    APP_ASSERT(service);
    APP_ASSERT(session_object);

    // find session object and destroy it along with its session record
    for (size_t i = 0; i < service->session_count; ++i) {
        app_service_session_t* session = &service->sessions[i];

        if ( session->session_object == session_object ) {
            cleanup_session(service, session);
            service->sessions[i] = service->sessions[--service->session_count];
            return;
        }
    }

    // if execution gets here, session_object does not belong to this service
    abort();
}
