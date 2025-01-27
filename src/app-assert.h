/// @file app-assert.h

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

#include "app-abort.h"


/**
 * Causes abort if given expression evaluates to zero/false.
 *
 * @param expr      Expression to evaulate.
 *
 * @note Does nothing if NDEBUG is defined.
 */
#define APP_ASSERT(expr)            APP_DETAIL_ASSERT(expr)


// PRIVATE
/// @cond DOXYGEN_IGNORE

#ifdef NDEBUG
#define APP_DETAIL_ASSERT(expr)             do {} while(0)
#else
#define APP_DETAIL_ASSERT(expr)             do { if (!(expr)) { \
                                                app_abort(APP_ABORT_REASON_ASSERTION_FAILURE, __LINE__); }} while(0)
#endif

/// @endcond
