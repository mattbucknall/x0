/// @file app-timeout.h

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

#include <stdint.h>


/**
 * Opaque structure representing a timeout period.
 */
typedef struct app_timeout app_timeout_t;


/**
 * Initialises and starts a timeout.
 *
 * @param timeout       Pointer to timeout object to initialise.
 *
 * @param period_ms     Period, in milliseconds, until timeout expires.
 */
void app_timeout_init(app_timeout_t* timeout, int64_t period_ms);


/**
 * Determines time until timeout object expires.
 *
 * @param timeout       Pointer to timeout object.
 *
 * @return Number of milliseconds remaining until timeout expires or zero if it has already expired.
 */
int64_t app_timeout_remaining_ms(const app_timeout_t* timeout);


// PRIVATE
/// @cond DOXYGEN_IGNORE

struct app_timeout {
    int64_t expiry;
};

/// @endcond
