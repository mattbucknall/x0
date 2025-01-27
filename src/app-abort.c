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

#include "app-abort.h"
#include "app-log.h"


const char *app_abort_reason_to_string(const app_abort_reason_t reason) {
    switch (reason) {
    case APP_ABORT_REASON_ASSERTION_FAILURE:
        return "assertion failure";

    case APP_ABORT_REASON_TYPE_MISMATCH:
        return "type mismatch";

    case APP_ABORT_REASON_ILLEGAL_BRANCH:
        return "illegal branch";

    case APP_ABORT_REASON_OUT_OF_MEMORY:
        return "out of memory";

    case APP_ABORT_REASON_ATEXIT_FAILED:
        return "atexit failed";

    case APP_ABORT_REASON_LUA_PANIC:
        return "lua panic";

    case APP_ABORT_REASON_UNHANDLED_ERROR:
        return "unhandled error";

    default:
        return "undefined abort reason code";
    }
}


noreturn void app_abort(app_abort_reason_t reason, uintptr_t metadata) {
    // dump abort reason and metadata
    app_log_fatal("ABORTED: %u: %s, %lu (0x%lx)",
                  reason, app_abort_reason_to_string(reason),
                  metadata, metadata
    );

    abort(); // no return
}
