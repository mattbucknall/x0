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
    app_service_session_ctx_t ctx; // must be first member
    struct app_service_session* prev;
    struct app_service_session* next;
    int client_socket;
    void* object;
} app_service_session_internal_t;


struct app_service {
    char* name;
    unsigned int max_connections;
    app_service_create_session_callback_t create_session_cb;
    app_service_destroy_session_callback_t destroy_session_cb;
    void* user_data;
    int listen_socket;
    app_event_id_t listen_id;
    app_service_session_internal_t* first_session;
    app_service_session_internal_t* last_session;
    unsigned int session_count;
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


static void cleanup_session(app_service_t* service, app_service_session_internal_t* session_internal) {
    APP_ASSERT(service);
    APP_ASSERT(session_internal);
    APP_ASSERT(session_internal->ctx.service == service);

    // destroy session object
    if ( session_internal->object ) {
        app_log_info("%s service closing connection from %s:%u", service->name,
                session_internal->ctx.client_addr_str, session_internal->ctx.client_port);

        service->destroy_session_cb(session_internal->object, service->user_data);
        session_internal->object = NULL;
    }

    // destroy stream
    if ( session_internal->ctx.stream ) {
        app_stream_destroy(session_internal->ctx.stream);
        session_internal->ctx.stream = NULL;
    }

    // close socket
    if ( session_internal->client_socket >= 0 ) {
        close_socket(session_internal->client_socket);
        session_internal->client_socket = -1;
    }

    // destroy session record
    session_internal->ctx.service = NULL;
    app_heap_free(session_internal);
}


static void accept_callback(uint32_t events, void* user_data) {
    app_service_t* service = user_data;

    // handle connection event
    if ( events & APP_EVENT_IN ) {
        int client_socket;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len;
        char addr_str[INET_ADDRSTRLEN];

        // accept connection
        do {
            client_socket = accept(service->listen_socket, (struct sockaddr*) &client_addr, &client_addr_len);
        } while(client_socket < 0 && errno == EINTR);

        if ( client_socket < 0 ) {
            // if accept failed, log warning
            app_log_warning("%s service unable to accept connection: %s", service->name, strerror(errno));
        } else if ( service->session_count >= service->max_connections ) {
            // reject connection if max sessions has been reached
            close_socket(client_socket);
        } else {
            app_service_session_internal_t* session_internal;

            // create session record
            session_internal = app_heap_alloc(sizeof(app_service_session_internal_t));

            session_internal->client_socket = client_socket;
            session_internal->ctx.stream = app_stream_create(client_socket, client_socket);

            // attempt to get client address
            if ( getpeername(client_socket, (struct sockaddr*) &client_addr, &client_addr_len) == 0 &&
                        inet_ntop(AF_INET, &(client_addr.sin_addr), session_internal->ctx.client_addr_str,
                        sizeof(session_internal->ctx.client_addr_str)) != NULL ) {
                session_internal->ctx.client_port = ntohs(client_addr.sin_port);
            } else {
                session_internal->ctx.client_addr_str[0] = '?';
                session_internal->ctx.client_addr_str[1] = '\0';
                session_internal->ctx.client_port = 0;
            }

            // create session object
            session_internal->object = service->create_session_cb(service, &session_internal->ctx, service->user_data);

            // log success and link session record into service's list on success or destroy on failure
            if ( session_internal->object ) {
                if ( service->last_session ) {
                    service->last_session->next = session_internal;
                } else {
                    service->first_session = session_internal;
                }

                session_internal->next = NULL;
                session_internal->prev = service->last_session;
                service->last_session = session_internal;

                app_log_info("%s service accepting connection from %s:%u",
                        service->name,
                        session_internal->ctx.client_addr_str,
                        session_internal->ctx.client_port);

                service->session_count++;
            } else {
                cleanup_session(service, session_internal);
            }
        }
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


void app_service_close_session(const app_service_session_ctx_t* ctx) {
    APP_ASSERT(ctx);

    app_service_t* service = ctx->service;
    app_service_session_internal_t* session_internal = (app_service_session_internal_t*) ctx;

    // unlink session record
    service->session_count--;

    if ( session_internal->next ) {
        session_internal->next->prev = session_internal->prev;
    } else {
        service->last_session = session_internal->prev;
    }

    if ( session_internal->prev ) {
        session_internal->prev->next = session_internal->next;
    } else {
        service->first_session = session_internal->next;
    }

    // destroy session
    cleanup_session(service, session_internal);
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
    service->listen_socket = listen_socket;
    service->listen_id = 0;
    service->first_session = NULL;
    service->last_session = NULL;
    service->session_count = 0;

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
        while(service->last_session) {
            app_service_close_session((app_service_session_ctx_t*) (service->last_session));
        }

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
