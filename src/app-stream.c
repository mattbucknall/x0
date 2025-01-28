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
#include <stdlib.h>

#include "app-assert.h"
#include "app-heap.h"
#include "app-stream.h"


struct app_stream {
    int read_fd;
    void* read_buffer;
    ssize_t read_n_bytes;
    app_stream_callback_t read_callback;
    void* read_user_data;
    app_event_id_t read_io_id;
    app_event_id_t read_timeout_id;

    int write_fd;
    const void* write_buffer;
    ssize_t write_n_bytes;
    app_stream_callback_t write_callback;
    void* write_user_data;
    app_event_id_t write_io_id;
    app_event_id_t write_timeout_id;
};


app_stream_t* app_stream_create(int read_fd, int write_fd) {
    APP_ASSERT(read_fd >= 0 || write_fd >= 0);

    app_stream_t* stream;

    // create new stream object
    stream = app_heap_alloc(sizeof(app_stream_t));

    stream->read_fd = read_fd;
    stream->read_buffer = NULL;
    stream->read_n_bytes = 0;
    stream->read_callback = NULL;
    stream->read_user_data = NULL;
    stream->read_io_id = 0;
    stream->read_timeout_id = 0;

    stream->write_fd = write_fd;
    stream->write_buffer = NULL;
    stream->write_n_bytes = 0;
    stream->write_callback = NULL;
    stream->write_user_data = NULL;
    stream->write_io_id = 0;
    stream->write_timeout_id = 0;

    return stream;
}


void app_stream_destroy(app_stream_t* stream) {
    app_heap_free(stream);
}


void app_stream_read(app_stream_t* stream, void* buffer, ssize_t n_bytes, app_stream_callback_t callback,
        void* user_data, app_timeout_t* timeout) {
    APP_ASSERT(stream);
    APP_ASSERT(buffer || n_bytes == 0);
    APP_ASSERT(callback);

    // ensure another read operaton is not in progress
    if ( stream->read_callback ) {
        abort(); // no return
    }

    // store read operation data
    stream->read_buffer = buffer;
    stream->read_n_bytes = n_bytes;
    stream->read_callback = callback;
    stream->read_user_data = NULL;
    stream->read_io_id = app_event_register_io(stream->read_fd, APP_EVENT_IN, read_io_callback, stream);
    stream->read_timeout_id = timeout ? app_event_register_timer(app_timeout_remaining_ms(timeout),
            read_timeout_callback, stream);
}


void app_stream_write(app_stream_t* stream, const void* buffer, ssize_t n_bytes, app_stream_callback_t callback,
        void* user_data, app_timeout_t* timeout) {
    APP_ASSERT(stream);
    APP_ASSERT(buffer || n_bytes == 0);
    APP_ASSERT(callback);

    // ensure another read operaton is not in progress
    if ( stream->write_callback ) {
        abort(); // no return
    }

    // store read operation data
    stream->write_buffer = buffer;
    stream->write_n_bytes = n_bytes;
    stream->write_callback = callback;
    stream->write_user_data = NULL;
    stream->write_io_id = app_event_register_io(stream->write_fd, APP_EVENT_IN, write_io_callback, stream);
    stream->write_timeout_id = timeout ? app_event_register_timer(app_timeout_remaining_ms(timeout),
            write_timeout_callback, stream);
}
