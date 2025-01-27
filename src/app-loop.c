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
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "app-abort.h"
#include "app-event.h"
#include "app-log.h"
#include "app-loop.h"

static volatile int m_result;
static volatile bool m_run_flag;


static void sig_callback(uint32_t events, void* user_data) {
    (void) events;
    (void) user_data;

    // cause main loop to stop
    app_loop_stop(EXIT_SUCCESS);
}


int app_loop_run(void) {
    int sig_fd;
    sigset_t sig_set;
    app_event_id_t sig_id;

    // use signalfd to monitor for terminating signals
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGINT);
    sigaddset(&sig_set, SIGTERM);
    sigaddset(&sig_set, SIGQUIT);
    
    sigprocmask(SIG_BLOCK, &sig_set, NULL);
    sig_fd = signalfd(-1, &sig_set, SFD_CLOEXEC);

    if ( sig_fd < 0 ) {
        app_log_error("Unable to create signal fd: %s", strerror(errno));
        app_abort(APP_ABORT_REASON_UNHANDLED_ERROR, 0); // no return
    }

    // register IO handler for signal fd
    sig_id = app_event_register_io(sig_fd, APP_EVENT_IN, sig_callback, NULL);

    // loop until run flag is cleared or error occurs
    m_run_flag = true;

    do {
        // process events
        app_event_poll(true);
    } while(m_run_flag);

    // unregister and close signal fd
    app_event_unregister_io(sig_id);

    while(close(sig_fd) < 0 && errno == EINTR) {
        // retry
    }

    return m_result;
}


void app_loop_stop(int result) {
    // set result code
    m_result = result;
    __sync_synchronize();

    // clear run flag, causing main loop to stop
    m_run_flag = false;
}
