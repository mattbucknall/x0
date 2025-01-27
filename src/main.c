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

#include "app-assert.h"
#include "app-event.h"
#include "app-log.h"
#include "app-loop.h"
#include "app-options.h"
#include "app-version.h"


int main(int argc, char* argv[]) {
    int exit_code = EXIT_FAILURE;

    // set runtime options using command-line arguments
    app_options_init(argc, argv); // may terminate

    // set minimum log priority
    app_log_set_min_priority(app_options_min_log_priority());

    // log start-up message
    app_log_info("x0 RV32IM Simulator - v" APP_VERSION_STR);

    // initialise event module
    app_event_init();

    // enter main loop
    exit_code = app_loop_run();

    // log shutdown message
    app_log_info("Terminating");

    return exit_code;
}
