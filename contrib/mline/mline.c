/*
 * mline - Lightweight UTF-8 aware line editing library.
 *
 * Copyright (C) 2025 Matthew T. Bucknall <matthew.bucknall@gmail.com>
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

#include "mline.h"

// assertion macros

#ifdef MLINE_NO_ASSERT
#define MLINE_ASSERT(expr)              do {} while(0)
#else

#ifndef MLINE_ABORT

#include <stdlib.h>

#define MLINE_ABORT                     abort()
#endif

#define MLINE_ASSERT(expr)              do { if ( !(expr) ) { MLINE_ABORT; } } while(0)
#endif


static void redraw(mline_ctx_t* ctx) {

}


void mline_feed(mline_ctx_t* ctx, const void* buffer, size_t n_bytes) {

}


void mline_refresh(mline_ctx_t* ctx) {
    MLINE_ASSERT(ctx);
    redraw(ctx);
}


void mline_set_prompt(mline_ctx_t* ctx, const char* prompt) {

}


void mline_set_width(mline_ctx_t* ctx, int columns) {

}


void mline_init(mline_ctx_t* ctx, char* line_buffer, size_t line_buffer_size, const char* prompt,
                int options, const mline_vtable_t* vtable, void* user_data) {
    MLINE_ASSERT(ctx);
    MLINE_ASSERT(line_buffer);
    MLINE_ASSERT(line_buffer_size > 0);
    MLINE_ASSERT(vtable);
    MLINE_ASSERT(vtable->write_callback);
    MLINE_ASSERT(vtable->proc_callback);
    MLINE_ASSERT(vtable->history_callback);

    ctx->vtable = vtable;
    ctx->user_data = user_data;
    ctx->line_buffer = line_buffer;
    ctx->line_buffer_size = line_buffer_size;
    ctx->discard_empty_lines = (options & MLINE_OPTION_DISCARD_EMPTY_LINES);
    ctx->history_state = 0;
    ctx->prompt_length = 0;
    ctx->width = 0;
    ctx->cursor_pos = 0;
    ctx->cursor_index = 0;

    mline_set_prompt(prompt);
}
