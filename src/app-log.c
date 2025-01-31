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

#include <stdio.h>
#include <stdlib.h>

#include "app-assert.h"
#include "app-log.h"
#include "app-version.h"


static app_log_priority_t m_min_priority;


const char* app_log_priority_to_string(app_log_priority_t priority) {
    switch(priority) {
    case APP_LOG_PRIORITY_DETAIL:
        return "detail";

    case APP_LOG_PRIORITY_INFO:
        return "info";

    case APP_LOG_PRIORITY_WARNING:
        return "warning";

    case APP_LOG_PRIORITY_ERROR:
        return "error";

    case APP_LOG_PRIORITY_FATAL:
        return "fatal";

    default:
        abort(); // no return
    }
}


void app_log_report_v(app_log_priority_t priority, const char* format, va_list args) {
    APP_ASSERT(priority >= APP_LOG_PRIORITY_DETAIL && priority <= APP_LOG_PRIORITY_FATAL);
    APP_ASSERT(format);

    // discard message if priority is lower than minimum priority
    if ( priority < m_min_priority ) {
        return;
    }

    // output message prologue
    fprintf(stderr, "\r[%-7s]: ", app_log_priority_to_string(priority));

    // output formatted message
    vfprintf(stderr, format, args);

    // output trailing newline character
    fputc('\n', stderr);
}


void app_log_report(app_log_priority_t priority, const char* format, ...) {
    APP_ASSERT(priority >= APP_LOG_PRIORITY_DETAIL && priority <= APP_LOG_PRIORITY_FATAL);
    APP_ASSERT(format);

    va_list args;

    va_start(args, format);
    app_log_report_v(priority, format, args);
    va_end(args);
}


void app_log_detail(const char* format, ...) {
    APP_ASSERT(format);

    va_list args;

    va_start(args, format);
    app_log_report_v(APP_LOG_PRIORITY_DETAIL, format, args);
    va_end(args);
}


void app_log_info(const char* format, ...) {
    APP_ASSERT(format);

    va_list args;

    va_start(args, format);
    app_log_report_v(APP_LOG_PRIORITY_INFO, format, args);
    va_end(args);
}


void app_log_warning(const char* format, ...) {
    APP_ASSERT(format);

    va_list args;

    va_start(args, format);
    app_log_report_v(APP_LOG_PRIORITY_WARNING, format, args);
    va_end(args);
}


void app_log_error(const char* format, ...) {
    APP_ASSERT(format);

    va_list args;

    va_start(args, format);
    app_log_report_v(APP_LOG_PRIORITY_ERROR, format, args);
    va_end(args);
}


void app_log_fatal(const char* format, ...) {
    APP_ASSERT(format);

    va_list args;

    va_start(args, format);
    app_log_report_v(APP_LOG_PRIORITY_FATAL, format, args);
    va_end(args);
}


static void cleanup(void) {
    // log termination message
    app_log_info("Terminating");
}


void app_log_init(app_log_priority_t priority) {
    APP_ASSERT(priority >= APP_LOG_PRIORITY_DETAIL && priority <= APP_LOG_PRIORITY_FATAL);

    // register cleanup function
    if ( atexit(cleanup) != 0 ) {
        abort(); // no return
    }

    // set minimum log priority
    m_min_priority = priority;

    // log version info
    app_log_info("x0 RV32IM Simulator - v" APP_VERSION_STR);
}
