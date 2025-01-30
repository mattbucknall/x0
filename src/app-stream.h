/// @file app-stream.h

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

#include <sys/types.h>

#include "app-event.h"
#include "app-result.h"
#include "app-timeout.h"


/**
 * Opaque type, holds context information for an asynchronous I/O stream.
 */
typedef struct app_stream app_stream_t;


/**
 * Function type for callbacks invoked when asynchronous I/O operations complete.
 *
 * @param stream            Stream on which operation completed.
 *
 * @param result            Operation result.
 *
 * @param n_transferred     Number of bytes read or written.
 *
 * @param user_data Opaque pointer passed to stream read/write function.
 */
typedef void (*app_stream_callback_t) (app_stream_t* stream, app_result_t result,
        ssize_t n_transferred, void* user_data);


/**
 * Creates a stream object, associating a file descriptors with it.
 *
 * @param read_fd   File descriptor to use for stream read operations.
 *
 * @param write_fd  File descriptor to use for stream write operations.
 *
 * @return  Pointer to new stream object (never NULL).
 *
 * @note    read_fd and write_fd can be passed the same descriptor. Passing a negative value to read_fd or write_fd
 *          makes the stream write-only or read-only respectively.
 */
app_stream_t* app_stream_create(int read_fd, int write_fd);


/**
 * Destroys a stream object. Does not close file dscriptors. Does nothing if stream is NULL.
 *
 * @param stream    Stream to destroy.
 */
void app_stream_destroy(app_stream_t* stream);


/**
 * Initiates an asynchronous read operation.
 *
 * @param stream    Stream to read from.
 *
 * @param buffer    Buffer to read data into.
 *
 * @param n_bytes   Maximum number of bytes to read (operation may read less).
 *
 * @param callback  Callback to invoke when operation completes.
 *
 * @param user_data Opaque pointer to pass to callback.
 *
 * @param timeout   Optional timeout.
 */
void app_stream_read(app_stream_t* stream, void* buffer, ssize_t n_bytes, app_stream_callback_t callback,
        void* user_data, const app_timeout_t* timeout);


/**
 * Initiates an asynchronous write operation.
 *
 * @param stream    Stream to write to.
 *
 * @param buffer    Buffer to write data from.
 *
 * @param n_bytes   Maximum number of bytes to write (operation may write less).
 *
 * @param callback  Callback to invoke when operation completes.
 *
 * @param user_data Opaque pointer to pass to callback.
 *
 * @param timeout   Optional timeout.
 */
void app_stream_write(app_stream_t* stream, const void* buffer, ssize_t n_bytes, app_stream_callback_t callback,
        void* user_data, const app_timeout_t* timeout);
