/// @file app-log.h

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

#include <stdarg.h>


/**
 * Enumeration of log priorities.
 */
typedef enum {
    APP_LOG_PRIORITY_DETAIL,
    APP_LOG_PRIORITY_INFO,
    APP_LOG_PRIORITY_WARNING,
    APP_LOG_PRIORITY_ERROR,
    APP_LOG_PRIORITY_FATAL
} app_log_priority_t;


/**
 * Provides representative string for given log priority.

 * @param priority  Log priority.
 *
 * @return  String representation of log priority.
 */
const char* app_log_priority_to_string(app_log_priority_t priority);


/**
 * Sets minimum log priority. Any messages logged with a priority less than that last passed to this function will be
 * discarded. Default minimum priority is APP_LOG_PRIORITY_DETAIL.
 *
 * @param priroity      Minimum log priority.
 *
 * @return  Previous log priority.
 */
app_log_priority_t app_log_set_min_priority(app_log_priority_t priority);


/**
 * Formats and writes given log message to stderr if given priority is equal or greater than minimum log priority set
 * by app_log_set_min_priority.
 *
 * @param priority      Log priority.
 *
 * @param format        Printf-style format string.
 *
 * @param args          List of arguments to supplant in formatted string.
 */
void app_log_report_v(app_log_priority_t priority, const char* format, va_list args)
        __attribute__((format(printf, 2, 0)));


/**
 * Formats and writes given log message to stderr is equal or greater than minimum log priority set by
 * app_log_set_min_priority.
 *
 * @param priority      Log priority.
 *
 * @param format        Printf-style format string.
 *
 * @param ...           Arguments to supplant in formatted string.
 */
void app_log_report(app_log_priority_t priority, const char* format, ...)
        __attribute__((format(printf, 2, 3)));


/**
 * Convenience function, calls app_log_report with APP_LOG_PRIORITY_DETAIL priority.
 *
 * @param format        Printf-style format string.
 *
 * @param ...           Arguments to supplant in formatted string.
 */
void app_log_detail(const char* format, ...);


/**
 * Convenience function, calls app_log_report with APP_LOG_PRIORITY_INFO priority.
 *
 * @param format        Printf-style format string.
 *
 * @param ...           Arguments to supplant in formatted string.
 */
void app_log_info(const char* format, ...);


/**
 * Convenience function, calls app_log_report with APP_LOG_PRIORITY_WARNING priority.
 *
 * @param format        Printf-style format string.
 *
 * @param ...           Arguments to supplant in formatted string.
 */
void app_log_warning(const char* format, ...);


/**
 * Convenience function, calls app_log_report with APP_LOG_PRIORITY_ERROR priority.
 *
 * @param format        Printf-style format string.
 *
 * @param ...           Arguments to supplant in formatted string.
 */
void app_log_error(const char* format, ...);


/**
 * Convenience function, calls app_log_report with APP_LOG_PRIORITY_FATAL priority.
 *
 * @param format        Printf-style format string.
 *
 * @param ...           Arguments to supplant in formatted string.
 */
void app_log_fatal(const char* format, ...);
