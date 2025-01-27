/// @file app-abort.h

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

#include <stdint.h>
#include <stdnoreturn.h>


/**
 * Enumeration of abort reason codes.
 */
typedef enum {
    APP_ABORT_REASON_ASSERTION_FAILURE,     ///< Metadata indicates line number
    APP_ABORT_REASON_TYPE_MISMATCH,         ///< Metadata indicates line number
    APP_ABORT_REASON_ILLEGAL_BRANCH,        ///< Metadata indicates line number
    APP_ABORT_REASON_OUT_OF_MEMORY,         ///< Metadata indicates attempted allocation amount
    APP_ABORT_REASON_ATEXIT_FAILED,         ///< Metadata indicates line number
    APP_ABORT_REASON_LUA_PANIC,             ///< No metadata
    APP_ABORT_REASON_UNHANDLED_ERROR        ///< No metadata
} app_abort_reason_t;


/**
 * Provides representative string for given abort reason code.
 *
 * @param reason    Reason code.
 * @return  Null-terminated string describing reason code.
 */
const char* app_abort_reason_to_string(const app_abort_reason_t reason);



/**
 * Performs abort, logging given reason code and metadata.
 *
 * @param reason    Reason for abort.
 *
 * @param metadata  Metadata to associate with reason for abort (meaning depends on abort reason).
 *
 * @note    Does not return.
 */
noreturn void app_abort(const app_abort_reason_t reason, const uintptr_t metadata);
