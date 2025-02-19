/*
 * NanoEdit (ned) - Lightweight UTF-8 aware line editing library.
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

#ifndef NED_H
#define NED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/**
 * Minimum allowable line buffer size.
 */
#define NED_MIN_BUFFER_SIZE         16


/**
 * Enumeration of result codes. Only `NED_RESULT_OK` indicates success; all other codes represent errors.
 */
typedef enum {
    NED_RESULT_OK,
    NED_RESULT_BUSY,
    NED_RESULT_WRITE_ERROR,
    NED_RESULT_PROMPT_TOO_LONG
} ned_result_t;


/**
 * Opaque type representing a nanoedit context.
 */
typedef struct ned ned_t;


/**
 * Callback function invoked by a nanoedit context to write output.
 *
 * @param buffer        Buffer containing data to output.
 *
 * @param buffer_size   Number of bytes in buffer.
 *
 * @param user_data     Opaque pointer passed to ned_init.
 *
 * @return  Number of bytes written (may be less than buffer_size) or -1 if a write error occurred.
 */
typedef int (*ned_write_callback_t) (const void* buffer, int buffer_size, void* user_data);


/**
 * Callback function invoked when a line read operation has completed, successfully or otherwise.
 *
 * @param result        Result code indicating success/failure of read operation.
 *
 * @param buffer        Buffer containing the edited line (or NULL if an error occurred).
 *
 * @param buffer_size   Number of bytes in buffer.
 *
 * @param user_data     Opaque pointer passed to ned_read.
 */
typedef void (*ned_read_callback_t) (ned_result_t result, const void* buffer, size_t buffer_size, void* user_data);


/**
 * Initialises a nanoedit context. Once a context is no longer required, it can be discarded - no
 * cleanup necessary. Any pending read line operation will not be completed if ned_feed is no longer
 * called on a context.
 *
 * @param ctx               Context to initialise.
 *
 * @param line_buffer       Buffer to store line contents.
 *
 * @param line_buffer_size  Size of buffer, in bytes. Must be at least NED_MIN_BUFFER_SIZE.
 *
 * @param write_callback    Function for context to use when it needs to output data.
 *
 * @param user_data         Opaque pointer to pass to write_callback.
 */
void ned_init(ned_t* ctx, void* buffer, size_t buffer_size, ned_write_callback_t write_callback, void* user_data);


/**
 * Feeds a context with input data. Input data should be fed to a context whenever it becomes available, regardless of
 * whether a read line operation is currently in progress.
 *
 * @param ctx           Context to feed data to.
 *
 * @param buffer        Buffer containing input data.
 *
 * @param buffer_size   Number of bytes in buffer.
 */
void ned_feed(ned_t* ctx, const void* buffer, size_t buffer_size);


/**
 * Reacquires terminla size and redraws editing session. Call this if terminal has changed size. This function should
 * be called any tme a resize occurs, regardless of whether a read line operation is currently in progress.
 */
void ned_redraw(ned_t* ctx);


/**
 * Begins an asynchronous read line operation. The given callback will be invoked when the operation completes,
 * successfully or otherwise. Calling this function invalidates any data passed to a previous invokation of a
 * read callback function from the given context.
 *
 * @param ctx               Context to perform read operation with.
 *
 * @param prompt            Prompt string to present user with.
 *
 * @param read_callback     Callback function to invoke when read operation finishes.
 *
 * @param user_data         Opaque pointer to pass to read_callback.
 *
 * @return      NED_RESULT_OK if read operation is successfully started, NED_RESULT_BUSY if another read operation is
 *              currently in progress, NED_RESULT_WRITE_ERROR if output data cannot be written,
 *              NED_RESULT_PROMPT_TOO_LONG if prompt cannot be buffered.
 */
ned_result_t ned_read_line(ned_t* ctx, const char* prompt, ned_read_callback_t read_callback, void* user_data);


/**
 * Returns a string representation of the given result code.
 *
 * @param result    The result code to convert.
 *
 * @return  A string describing the result code.
 */
const char* ned_result_to_string(ned_result_t result);


/** @cond nanoedit_private */

#ifdef NED_NO_ASSERT
#define NED_ASSERT(expr)            do {} while(0)
#else // NED_NO_ASSERT

#ifndef NED_ABORT
#include <stdlib.h>
#define NED_ABORT                   abort()
#endif // NED_ABORT

#define NED_ASSERT(expr)            do { if ( !(expr) ) { NED_ABORT; } } while(0)

#endif // NED_NO_ASSERT


#ifndef NED_OUTPUT_BUFFER_SIZE
#define NED_OUTPUT_BUFFER_SIZE      256
#endif // NED_OUTPUT_BUFFER_SIZE


typedef enum {
    NED_LEX_STATE_ASCII,
    NED_LEX_STATE_ESC_TYPE,
    NED_LEX_STATE_SS3,
    NED_LEX_STATE_CSI,
    NED_LEX_STATE_IGNORED_CSI
} ned_lex_state_t;


struct ned {
    uint8_t* input_buffer;
    size_t input_buffer_size;
    size_t input_buffer_start;
    size_t input_buffer_idx;
    uint8_t output_buffer[NED_OUTPUT_BUFFER_SIZE];
    size_t output_buffer_idx;
    ned_write_callback_t write_callback;
    void* write_user_data;
    uint16_t terminal_width;
    uint16_t terminal_height;
    uint16_t cursor_start;
    uint16_t cursor_col;
    uint16_t cursor_row;
    ned_read_callback_t read_callback;
    void* read_user_data;
    ned_lex_state_t lex_state;
    uint8_t lex_utf8_pending;
    uint8_t lex_idx;
    uint32_t lex_arg[2];

#ifndef NED_NO_ASSERT
    uint32_t tag;
#endif // NED_NO_ASSERT
};

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif // NED_H
