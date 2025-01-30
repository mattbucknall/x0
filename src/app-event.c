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

#define _POSIX_C_SOURCE 199309L

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app-assert.h"
#include "app-event.h"
#include "app-heap.h"
#include "app-log.h"


typedef struct {
    app_event_id_t id;
    app_event_io_callback_t callback;
    void* user_data;
} app_event_io_t;


typedef struct {
    app_event_id_t id;
    app_event_timer_callback_t callback;
    void* user_data;
    int64_t expiry;
} app_event_timer_t;


static app_event_id_t m_id_counter;
static app_event_io_t* m_io_records;
static struct pollfd* m_io_pfds;
static size_t m_io_record_count;
static size_t m_io_record_capacity;
static app_event_timer_t* m_timer_records;
static size_t m_timer_record_count;
static size_t m_timer_record_capacity;


int64_t app_event_clock(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    return (int64_t) ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}


app_event_id_t app_event_register_io(int fd, uint32_t events, app_event_io_callback_t callback, void* user_data) {
    APP_ASSERT(fd >= 0);
    APP_ASSERT(events);
    APP_ASSERT(callback);

    app_event_io_t* io_record;
    struct pollfd* io_pfd;

    // ensure there is space for new IO record and pfd in their respective lists
    if ( m_io_record_count == m_io_record_capacity ) {
        m_io_record_capacity *= 2;
        m_io_records = app_heap_realloc(m_io_records, m_io_record_capacity * sizeof(app_event_io_t));
        m_io_pfds = app_heap_realloc(m_io_pfds, m_io_record_capacity * sizeof(struct pollfd));
    }

    // create new IO record
    io_record = &m_io_records[m_io_record_count];
    io_record->id = ++m_id_counter;
    io_record->callback = callback;
    io_record->user_data = user_data;

    // create new pfd
    io_pfd = &m_io_pfds[m_io_record_count++];
    io_pfd->fd = fd;
    io_pfd->events = events;
    io_pfd->revents = 0;

    // return handler ID
    return io_record->id;
}


void app_event_unregister_io(app_event_id_t id) {
    // find IO record with given ID and mark it for garbage collecton
    for (size_t i = 0; i < m_io_record_count; ++i) {
        if ( m_io_records[i].id == id ) {
            m_io_records[i].id = 0;
            break;
        }
    }
}


static void gc_io(void) {
    // remove all IO records (and their associated pfds) marked fo garbage collection
    for (size_t i = 0; i < m_io_record_count;) {
        if ( m_io_records[i].id == 0 ) {
            m_io_records[i] = m_io_records[--m_io_record_count];
            m_io_pfds[i] = m_io_pfds[m_io_record_count];
        } else {
            ++i;
        }
    }
}


app_event_id_t app_event_register_timer(int64_t period, app_event_timer_callback_t callback, void* user_data) {
    APP_ASSERT(period >= 0);
    APP_ASSERT(callback);

    app_event_timer_t* timer;

    // ensure there is space for new timer record in list
    if ( m_timer_record_count == m_timer_record_capacity ) {
        m_timer_record_capacity *= 2;
        m_timer_records = app_heap_realloc(m_timer_records, m_timer_record_capacity * sizeof(app_event_timer_t));
    }

    // create new timer record
    timer = &m_timer_records[m_timer_record_count++];
    timer->id = ++m_id_counter;
    timer->callback = callback;
    timer->user_data = user_data;
    timer->expiry = app_event_clock() + period;

    // return handler ID
    return timer->id;
}


void app_event_unregister_timer(app_event_id_t id) {
    // find timer record with given ID and mark it for garbage collection
    for (size_t i = 0; i < m_timer_record_count; ++i) {
        if ( m_timer_records[i].id == id ) {
            m_timer_records[i].id = 0;
        }
    }
}


static void gc_timers(void) {
    // remove all timer records marked for garbage collection
    for (size_t i = 0; i < m_timer_record_count;) {
        if ( m_timer_records[i].id == 0 ) {
            m_timer_records[i] = m_timer_records[--m_timer_record_count];
        } else {
            ++i;
        }
    }
}


void app_event_poll(bool block) {
    int poll_result;
    int64_t now;

    // perform garbage collection
    gc_io();
    gc_timers();

    // ensure pfd revents are reset
    for (size_t i = 0; i < m_io_record_count; ++i) {
        m_io_pfds[i].revents = 0;
    }

    // poll for events, handling any signal handler interruptions
    do {
        int64_t timeout;

        // calculate poll timeout period
        if ( block ) {
            if ( m_timer_record_count == 0 ) {
                // if no timers, then block indefinitely
                timeout = -1;
            } else {
                // timeout when earliest timer expiry is due
                now = app_event_clock();

                timeout = INT64_MAX;

                for (size_t i = 0; i < m_timer_record_count; ++i) {
                    int64_t remaining_ms = m_timer_records[i].expiry - now;

                    if ( remaining_ms < 0 ) {
                        timeout = 0;
                    } else if ( remaining_ms < timeout ) {
                        timeout = remaining_ms;
                    }
                }
            }
        } else {
            // do not block
            timeout = 0;
        }

        // perform poll
        if ( timeout > INT_MAX ) {
            timeout = INT_MAX;
        }

        poll_result = poll(m_io_pfds, m_io_record_count, (int) timeout);
    } while(poll_result < 0 && errno == EINTR);

    // check for poll errors
    if ( poll_result < 0 ) {
        app_log_fatal("Event polling error: %s", strerror(errno));
        abort();
    }

    // dispatch triggered IO event handlers
    for (size_t i = 0; i < m_io_record_count; ++i) {
        if ( m_io_pfds[i].revents && m_io_records[i].id ) {
            // mark IO record for garbage collection
            m_io_records[i].id = 0;

            // dispatch handler
            m_io_records[i].callback(m_io_pfds[i].revents, m_io_records[i].user_data);
        }
    }

    // dispatch expired timer event handlers
    now = app_event_clock();

    for (size_t i = 0; i < m_timer_record_count; ++i) {
        if ( m_timer_records[i].expiry <= now && m_timer_records[i].id ) {
            // mark timer record for garbage collection
            m_timer_records[i].id = 0;

            // dispatch handler
            m_timer_records[i].callback(m_timer_records[i].user_data);
        }
    }
}


static void cleanup(void) {
    // free IO records
    app_heap_free(m_io_records);
    m_io_records = NULL;
    m_io_record_count = 0;
    m_io_record_capacity = 0;

    // free timer records
    app_heap_free(m_timer_records);
    m_timer_records = NULL;
    m_timer_record_count = 0;
    m_timer_record_capacity = 0;
}


void app_event_init(void) {
    // allocate initial IO record and pfd lists
    m_io_record_capacity = 16;
    m_io_records = app_heap_alloc(m_io_record_capacity * sizeof(app_event_io_t));
    m_io_pfds = app_heap_alloc(m_io_record_capacity * sizeof(struct pollfd));

    // allocate initial timer record list
    m_timer_record_capacity = 16;
    m_timer_records = app_heap_alloc(m_timer_record_capacity * sizeof(app_event_timer_t));

    // register cleanup function
    if ( atexit(cleanup) != 0 ) {
        abort(); // no return
    }
}
