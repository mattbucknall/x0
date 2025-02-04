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

#ifndef MLINE_H
#define MLINE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Opaque type, representing a line editor context.
 */
typedef struct mline_ctx mline_ctx_t;


/**
 * Function pointer type for callback function used to synchronously write line editor output.
 *
 * @param buffer    Buffer containing data to write.
 *
 * @param n_bytes   Number of bytes in buffer to write.
 *
 * @param user_data Opaque pointer passed to mline_init.
 *
 * @return  Number of bytes written (may be less than n_bytes) or -1 to indicate error.
 */
typedef int (* mline_write_func_t)(const void* buffer, int n_bytes, void* user_data);


/**
 * Function pointer type for callback function used to process line.
 *
 * @param line          Null-terminated line.
 *
 * @param length        Length of line, in bytes, excluded null-termination character.
 *
 * @param user_data     Opaque pointer passed to mline_init.
 */
typedef void (* mline_proc_func_t)(const char* line, size_t length, void* user_data);


/**
 * Function pointer type for callback function used to get history content.
 *
 * @param index         Value indicating how far back in history content should be fetched for.
 *
 * @param user_data     Opaque pointer passed to mline_init.
 *
 * @return  Null-terminated string containing indexed line of history, NULL if no such history exists.
 */
typedef const char* (* mline_history_func_t)(int index, void* user_data);


/**
 * Table of callback functions.
 */
typedef struct {
    mline_write_func_t write_callback;
    mline_proc_func_t proc_callback;
    mline_history_func_t history_callback;
} mline_vtable_t;


/**
 * Option flag - Causes line editor to discard empty lines.
 */
#define MLINE_OPTION_DISCARD_EMPTY_LINES            (1 << 0)

/**
 * Initialises a line editor context.
 *
 * @param ctx               Pointer to context structure to initialise.
 *
 * @param line_buffer       Pointer to buffer for context to store line data in.
 *
 * @param line_buffer_size  Size of line buffer in bytes.
 *
 * @param prompt            Initial prompt (copied into context, so does not need to persist).
 *                          If NULL, no prompt will be used.
 *
 * @param options           Bitwise-OR of option flags.
 *
 * @param vtable            Pointer to table of callback functions for context to use (must persist for life time
 *                          of context).
 *
 * @param user_data         Opaque pointer to pass to callback functions.
 */
void mline_init(mline_ctx_t* ctx, char* line_buffer, size_t line_buffer_size, const char* prompt,
                int options, const mline_vtable_t* vtable, void* user_data);


/**
 * Feeds line editor context with input, which may cause output to refresh.
 *
 * @param ctx       Line editor context.
 *
 * @param buffer    Buffer containing input data.
 *
 * @param n_bytes   Number of bytes in buffer.
 */
void mline_feed(mline_ctx_t* ctx, const void* buffer, size_t n_bytes);


/**
 * Refreshes output.
 *
 * @param ctx       Line editor context.
 */
void mline_refresh(mline_ctx_t* ctx);

/**
 * Sets prompt and refreshes output.
 *
 * @param ctx       Line editor context.
 *
 * @param prompt    Prompt string (coped into context, so does not need to persist).
 *                  If NULL, no prompt will be used.
 */
void mline_set_prompt(mline_ctx_t* ctx, const char* prompt);


/**
 * Sets terminal width and refreshes output.
 *
 * @param ctx       Line editor context.
 *
 * @param columns   Terminal with in columns.
 */
void mline_set_width(mline_ctx_t* ctx, int columns);


/** @cond mline_doxygen_exclude */

struct mline_ctx {
    const mline_vtable_t* vtable;
    void* user_data;
    char* line_buffer;
    size_t line_buffer_size;
    bool discard_empty_lines;
    int history_state;
    size_t prompt_length;
    int width;
    int cursor_pos;
    size_t cursor_index;
};

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif /* MLINE_H */
