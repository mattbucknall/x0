/// @file app-service.h

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

#pragma once

#include <stdint.h>

#include <netinet/in.h>

#include "app-stream.h"


/**
 * Opaque type, holds content information for a TCP service.
 */
typedef struct app_service app_service_t;


/**
 * Contextual data for each session started by a service.
 */
typedef struct {
    app_service_t* service;                 ///< Service that created session.
    app_stream_t* stream;                   ///< Stream for session to perform I/O with.
    char client_addr_str[INET_ADDRSTRLEN];  ///< String containing client's address.
    uint16_t client_port;                   ///< Client's port number
} app_service_session_ctx_t;


/**
 * Function type for callback invoked by a service to create a session object serving a new connection.
 *
 * @param service   Service invoking the function.
 *
 * @param ctx       Session data.
 *
 * @param user_data Opaque pointer passed to app_service_create.
 *
 * @return  Pointer to session object or NULL if session was not created.
 */
typedef void* (*app_service_create_session_callback_t) (app_service_t* service,
        const app_service_session_ctx_t* ctx, void* user_data);


/**
 * Function type for callback invoked by a service when it needs to destroy a session object.
 *
 * @param session_object    Pointer to session object.
 *
 * @param user_data         Opaque pointer passed to app_service_create.
 */
typedef void (*app_service_destroy_session_callback_t) (void* session_object, void* user_data);


/**
 * Creates a TCP service bound to given address.
 *
 * @param name      Service's name (service will make its own copy if this string).
 *
 * @param addr      Address/port to bind to.
 *
 * @param max_conns Maximum number of simultanious connections service shall accept.
 *
 * @param create_session_cb     Callback to invoke when a new session needs to be created, to service new connection.
 *
 * @param destroy_session_cb    Callback to invoke when a session needs to be destroyed.
 *
 * @param user_data             Opaque pointer to pass to callback functions.
 *
 * @return  Pointer to newly created service object or NULL if service could not bind to given address.
 */
app_service_t* app_service_new(const char* name, const struct sockaddr_in* addr, unsigned int max_conns,
        app_service_create_session_callback_t create_session_cb,
        app_service_destroy_session_callback_t destroy_session_cb,
        void* user_data);


/**
 * Destroys a TCP service and any of its action sessions.
 *
 * @param service   Service to destroy.
 */
void app_service_destroy(app_service_t* service);


/**
 * Destroys a session object belonging to a service. Session implementations may use this function to destroy
 * themselves when, for example, their remote peer has closed its connection.
 *
 * @param ctx               Context for session to be closed.
 *
 * @note    Calling this function invalidates session data.
 */
void app_service_close_session(const app_service_session_ctx_t* ctx);
