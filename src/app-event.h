/// @file app-event.h

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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <poll.h>


#define APP_EVENT_IN    (POLLIN)        ///< Event flag indicating file descriptor is readable.
#define APP_EVENT_OUT   (POLLOUT)       ///< Event flag indicating file descriptor is writable.
#define APP_EVENT_ERR   (POLLERR)       ///< Event flag indicating file descriptor associated error occurred.
#define APP_EVENT_HUP   (POLLHUP)       ///< Event flag indicating hang up occurred on file descriptor.


/**
 * Type used to represent event handler IDs. Event handler IDs are non-zero and never reused.
 */
typedef uint64_t app_event_id_t;


/**
 * Callback type invoked when an event occurs.
 *
 * @param event     Bitwise-OR of event flags indicating reason for invoking callback. Will be zero for timer events.
 *
 * @param user_data Opaque pointer passed to handler registration function.
 */
typedef void (*app_event_callback_t) (uint32_t events, void* user_data);


/**
 * Initialises event module.
 */
void app_event_init(void);


/**
 * @return      Event system monotonic clock time in milliseconds since epoch.
 */
int64_t app_event_clock(void);


/**
 * Registers an I/O event handler.
 *
 * @param fd        File descriptor to monitor for events.
 *
 * @param events    Bitwise-OR of event flags indicating events to monitor file descriptor for.
 *
 * @param callback  Callback function to invoke when one or more specified events occur.
 *
 * @param user_data Opaque pointer to pass to callback.
 *
 * @return  Event handler ID.
 */
app_event_id_t app_event_register_io(int fd, uint32_t events, app_event_callback_t callback, void* user_data);


/**
 * Unregisters an I/O event handler, preventing it from being invoked. Does nothing if given ID is invalid/expended.
 *
 * @param id    Event handler ID.
 */
void app_event_unregister_io(app_event_id_t id);


/**
 * Registers a timer event handler.
 *
 * @param period    Period to wait before invoking event handler.
 *
 * @param callback  Callback function to invoke when time period has expired.
 *
 * @param user_data Opaque pointer to pass to callback.
 *
 * @return  Event handler ID.
 */
app_event_id_t app_event_register_timer(int64_t period, app_event_callback_t callback, void* user_data);


/**
 * Unregisters a timer event handler, preventing it from being invoked. Does nothing if given ID is invalid/expended.
 *
 * @param id    Event handler ID.
 */
void app_event_unregister_timer(app_event_id_t id);


/**
 * Optionally waits for, then processes any pending events and returns.
 *
 * @param block     If true, blocks whilst no events are pending.
 */
void app_event_poll(bool block);
