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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "ned.h"


#define NED_TAG                 0x4E454421ul

#ifndef NED_NO_ASSERT
inline static bool validate_ctx(ned_t* ctx) {
    return ctx && ctx->tag == NED_TAG;
}
#endif // NED_NO_ASSERT

#define NED_ASSERT_CTX(ctx)     NED_ASSERT(validate_ctx(ctx))

#define NED_CO_BEGIN(state)     switch(state) {case 0:
#define NED_CO_YIELD(state)     state = __LINE__; case __LINE__:
#define NED_CO_END(state)       } state = 0


#define NED_ASCII_ETX           (0x03)
#define NED_ASCII_LF            (0x0A)
#define NED_ASCII_CR            (0x0D)
#define NED_ASCII_ESC           (0x1B)
#define NED_ASCII_DEL           (0x7F)


typedef enum {
    NED_TOKEN_TYPE_CODE_POINT,
    NED_TOKEN_TYPE_CURSOR_POSITION,
    NED_TOKEN_TYPE_CURSOR_UP,
    NED_TOKEN_TYPE_CURSOR_DOWN,
    NED_TOKEN_TYPE_CURSOR_LEFT,
    NED_TOKEN_TYPE_CURSOR_RIGHT,
    NED_TOKEN_TYPE_ENTER,
    NED_TOKEN_TYPE_BACKSPACE,
    NED_TOKEN_TYPE_DELETE,
    NED_TOKEN_TYPE_HOME,
    NED_TOKEN_TYPE_END,
    NED_TOKEN_TYPE_END_OF_TEXT
} ned_token_type_t;


typedef struct {
    ned_token_type_t type;

    union {
        uint32_t code_point;

        struct {
            uint16_t row;
            uint16_t col;
        } position;
    } data;
} ned_token_t;


const char* ned_result_to_string(ned_result_t result) {
    switch(result) {
    case NED_RESULT_OK:                 return "ok";
    case NED_RESULT_BUSY:               return "busy";
    case NED_RESULT_WRITE_ERROR:        return "write error";
    case NED_RESULT_PROMPT_TOO_LONG:    return "prompt too long";
    default:                            return "unknown result";
    }
}


static uint16_t estimate_terminal_columns(uint32_t code_point) {
    // control characters
    if (code_point < 0x20 || (code_point >= 0x7F && code_point < 0xA0)) {
        return 0;
    }

    // printable ASCII
    if (code_point < 0x7F) {
        return 1;
    }

    // combining characters
    if ((code_point >= 0x0300 && code_point <= 0x036F) ||       // Combining Diacritical Marks
        (code_point >= 0x1AB0 && code_point <= 0x1AFF) ||
        (code_point >= 0x1DC0 && code_point <= 0x1DFF) ||
        (code_point >= 0x20D0 && code_point <= 0x20FF) ||
        (code_point >= 0xFE20 && code_point <= 0xFE2F)) {
        return 0;
    }

    // wide characters (CJK, Emoji, Fullwidth)
    if ((code_point >= 0x1100 && code_point <= 0x115F) ||       // Hangul Jamo
        (code_point >= 0x2E80 && code_point <= 0xA4CF) ||       // CJK, Yi
        (code_point >= 0xAC00 && code_point <= 0xD7A3) ||       // Hangul Syllables
        (code_point >= 0xF900 && code_point <= 0xFAFF) ||       // CJK Compatibility
        (code_point >= 0xFE10 && code_point <= 0xFE19) ||       // Vertical punctuation
        (code_point >= 0xFE30 && code_point <= 0xFE6F) ||       // Fullwidth forms
        (code_point >= 0xFF00 && code_point <= 0xFF60) ||       // Fullwidth ASCII variants
        (code_point >= 0x1F300 && code_point <= 0x1F64F) ||     // Emojis (e.g., smileys)
        (code_point >= 0x1F900 && code_point <= 0x1F9FF)) {     // Supplemental Symbols
        return 2;
    }

    // assume everything else is one column wide
    return 1;
}


static void output_clear(ned_t* ctx) {
    ctx->output_buffer_idx = 0;
}


static ned_result_t output_flush(ned_t* ctx) {
    const uint8_t* buffer_i = ctx->output_buffer;
    const uint8_t* buffer_e = buffer_i + ctx->output_buffer_idx;

    ctx->output_buffer_idx = 0;

    while(buffer_i < buffer_e) {
       int n_written = ctx->write_callback(buffer_i, buffer_e - buffer_i, ctx->write_user_data);

       if ( n_written < 0 ) {
            return NED_RESULT_WRITE_ERROR;
       }

       buffer_i += n_written;
    }

    return NED_RESULT_OK;
}


static ned_result_t output(ned_t* ctx, const void* buffer, size_t buffer_size) {
    const uint8_t* buffer_i = buffer;
    const uint8_t* buffer_e = buffer_i + buffer_size;

    while(buffer_i < buffer_e) {
        size_t pending = buffer_e - buffer_i;
        size_t space = NED_OUTPUT_BUFFER_SIZE - ctx->output_buffer_idx;

        if ( pending > space ) {
            pending = space;
        }

        memcpy(&ctx->output_buffer[ctx->output_buffer_idx], buffer_i, pending);
        ctx->output_buffer_idx += pending;
        buffer_i += pending;

        if ( ctx->output_buffer_idx == NED_OUTPUT_BUFFER_SIZE ) {
            ned_result_t result = output_flush(ctx);

            if ( result != NED_RESULT_OK ) {
                return result;
            }
        }
    }

    return NED_RESULT_OK;
}


static ned_result_t output_uint16(ned_t* ctx, uint16_t value) {
    char temp[5];
    int i = 0;

    // handle zero case
    if ( value == 0 ) {
        temp[0] = '0';
        return output(ctx, temp, 1);
    }

    // convert digits to string (in reverse)
    while (value > 0) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    // reverse string
    for (int j = 0; j < (i / 2); ++j) {
        char t = temp[j];
        temp[j] = temp[i - j - 1];
        temp[i - j - 1] = t;
    }

    // output value
    return output(ctx, temp, i);
}


static ned_result_t move_cursor(ned_t* ctx, char command, uint16_t n) {
    ned_result_t result;

    result = output(ctx, "\x1B[", 2);

    if ( result != NED_RESULT_OK ) {
        return result;
    }

    result = output_uint16(ctx, n);

    if ( result != NED_RESULT_OK ) {
        return result;
    }

    return output(ctx, &command, 1);
}


static ned_result_t move_cursor_up(ned_t* ctx, uint16_t n_rows) {
    return move_cursor(ctx, 'A', n_rows);
}


static ned_result_t move_cursor_down(ned_t* ctx, uint16_t n_rows) {
    return move_cursor(ctx, 'B', n_rows);
}


static ned_result_t move_cursor_forward(ned_t* ctx, uint16_t n_columns) {
    return move_cursor(ctx, 'C', n_columns);
}


static ned_result_t move_cursor_backward(ned_t* ctx, uint16_t n_columns) {
    return move_cursor(ctx, 'D', n_columns);
}


static ned_result_t hide_cursor(ned_t* ctx) {
    return output(ctx, "\x1B[?25l", 6);
}


static ned_result_t show_cursor(ned_t* ctx) {
    return output(ctx, "\x1B[?25h", 6);
}


static ned_result_t erase_line_from_cursor(ned_t* ctx) {
    return output(ctx, "\x1B[K", 3);
}


static ned_result_t move_cursor_to_bottom_right(ned_t* ctx) {
    return output(ctx, "\x1B[9999;9999H", 12);
}


static ned_result_t request_cursor_position(ned_t* ctx) {
    return output(ctx, "\x1B[6n", 4);
}


static void lex_reset(ned_t* ctx) {
    ctx->lex_state = NED_LEX_STATE_ASCII;
    ctx->lex_utf8_pending = 0;
    ctx->lex_idx = 0;
    ctx->lex_arg[0] = 0;
    ctx->lex_arg[1] = 0;
}


static void handle_token(ned_t* ctx, const ned_token_t* token) {
    // reset lex state
    lex_reset(ctx);

    switch(token->type) {
    case NED_TOKEN_TYPE_CODE_POINT:
        printf("U+%04X\n", token->data.code_point);
        break;

    case NED_TOKEN_TYPE_CURSOR_POSITION:
        printf("Cursor Pos: %u, %u\n", token->data.position.col, token->data.position.row);
        break;

    case NED_TOKEN_TYPE_CURSOR_UP:
        printf("Cursor Up\n");
        break;

    case NED_TOKEN_TYPE_CURSOR_DOWN:
        printf("Cursor Down\n");
        break;

    case NED_TOKEN_TYPE_CURSOR_LEFT:
        printf("Cursor Left\n");
        break;

    case NED_TOKEN_TYPE_CURSOR_RIGHT:
        printf("Cursor Right\n");
        break;

    case NED_TOKEN_TYPE_ENTER:
        printf("Enter\n");
        break;

    case NED_TOKEN_TYPE_BACKSPACE:
        printf("Backspace\n");
        break;

    case NED_TOKEN_TYPE_DELETE:
        printf("Delete\n");
        break;

    case NED_TOKEN_TYPE_HOME:
        printf("Home\n");
        break;

    case NED_TOKEN_TYPE_END:
        printf("End\n");
        break;

    case NED_TOKEN_TYPE_END_OF_TEXT:
        printf("End-of-Text\n");
        break;

    default:
        break;
    }
}


static void lex_step(ned_t* ctx, uint8_t byte) {
    ned_token_t token;

    // inversion of control nightmare!

    if ( byte == NED_ASCII_ETX ) { // Ctrl+C pressed
        token.type = NED_TOKEN_TYPE_END_OF_TEXT;
        handle_token(ctx, &token);
    } else if ( byte == NED_ASCII_LF || byte == NED_ASCII_CR ) { // Enter pressed
        token.type = NED_TOKEN_TYPE_ENTER;
        handle_token(ctx, &token);
    } else if ( byte == NED_ASCII_DEL ) { // Backspace pressed
        token.type = NED_TOKEN_TYPE_BACKSPACE;
        handle_token(ctx, &token);
    } else if ( byte == NED_ASCII_ESC ) { // Start of escape sequence (or ESC pressed)
        ctx->lex_state = NED_LEX_STATE_ESC_TYPE;
    } else if ( byte < ' ' ) { // Ignored ASCII control character received
        lex_reset(ctx);
    } else if ( (byte & 0xE0) == 0xC0 ) { // Start of 2-byte UTF-8 sequence
        ctx->lex_state = NED_LEX_STATE_ASCII;
        ctx->lex_utf8_pending = 1;
        ctx->lex_arg[0] = byte & 0x1F;
    } else if ( (byte & 0xF0) == 0xE0 ) { // Start of 3-byte UTF-8 sequence
        ctx->lex_state = NED_LEX_STATE_ASCII;
        ctx->lex_utf8_pending = 2;
        ctx->lex_arg[0] = byte & 0x0F;
    } else if ( (byte & 0xF8) == 0xF0 ) { // Start of 4-byte UTF-8 sequence
        ctx->lex_state = NED_LEX_STATE_ASCII;
        ctx->lex_utf8_pending = 3;
        ctx->lex_arg[0] = byte & 0x07;
    } else if ( (byte & 0xC0) == 0x80 ) { // Continuation of multi-byte UTF-8 sequence
        // ignore byte if not expecting multi-byte UTF-8 sequence byte
        if ( ctx->lex_utf8_pending > 0 ) {
            ctx->lex_arg[0] = (ctx->lex_arg[0] << 6) | (byte & 0x3F);
            ctx->lex_utf8_pending--;

            if ( ctx->lex_utf8_pending == 0 ) {
                token.type = NED_TOKEN_TYPE_CODE_POINT;
                token.data.code_point = ctx->lex_arg[0];
                handle_token(ctx, &token);
            }
        } else {
            lex_reset(ctx);
        }
    } else if ( (byte & 0x80) == 0x00 ) { // ASCII character (or continuation of escape sequence)
        ctx->lex_utf8_pending = 0;

        switch(ctx->lex_state) {
        case NED_LEX_STATE_ASCII:
            token.type = NED_TOKEN_TYPE_CODE_POINT;
            token.data.code_point = byte;
            handle_token(ctx, &token);
            break;

        case NED_LEX_STATE_ESC_TYPE:
            if ( byte == '[' ) { // CSI style escape sequence
                ctx->lex_state = NED_LEX_STATE_CSI;
                ctx->lex_idx = 0;
                ctx->lex_arg[0] = 0;
                ctx->lex_arg[1] = 0;
            } else if ( byte == 'O' ) { // SS3 style escape sequence
                ctx->lex_state = NED_LEX_STATE_SS3;
            } else {
                lex_reset(ctx);
            }
            break;

        case NED_LEX_STATE_SS3:
            if ( byte == 'A' ) {
                token.type = NED_TOKEN_TYPE_CURSOR_UP;
                handle_token(ctx, &token);
            } else if ( byte == 'B' ) {
                token.type = NED_TOKEN_TYPE_CURSOR_DOWN;
                handle_token(ctx, &token);
            } else if ( byte == 'C' ) {
                token.type = NED_TOKEN_TYPE_CURSOR_RIGHT;
                handle_token(ctx, &token);
            } else if ( byte == 'D' ) {
                token.type = NED_TOKEN_TYPE_CURSOR_LEFT;
                handle_token(ctx, &token);
            } else if ( byte == 'H' ) {
                token.type = NED_TOKEN_TYPE_HOME;
                handle_token(ctx, &token);
            } else if ( byte == 'F' ) {
                token.type = NED_TOKEN_TYPE_END;
                handle_token(ctx, &token);
            } else {
                lex_reset(ctx);
            }
            break;

        case NED_LEX_STATE_CSI:
            if ( byte == ' ' ) {
                // ignore
            } else if ( byte >= '0' && byte <= '9' ) { // decode numeric argument
                ctx->lex_arg[ctx->lex_idx] = (ctx->lex_arg[ctx->lex_idx] * 10) + (byte - '0');
            } else if ( byte == ';' ) { // advance to next argument (ignore sequence if more than two arguments)
                ctx->lex_idx++;

                if ( ctx->lex_idx >= 2 ) {
                    ctx->lex_state = NED_LEX_STATE_IGNORED_CSI;
                }
            } else if ( byte >= '@' && byte <= '~' ) { // process complete sequence
                if ( byte == 'A' ) {
                    token.type = NED_TOKEN_TYPE_CURSOR_UP;
                    handle_token(ctx, &token);
                } else if ( byte == 'B' ) {
                    token.type = NED_TOKEN_TYPE_CURSOR_DOWN;
                    handle_token(ctx, &token);
                } else if ( byte == 'C' ) {
                    token.type = NED_TOKEN_TYPE_CURSOR_RIGHT;
                    handle_token(ctx, &token);
                } else if ( byte == 'D' ) {
                    token.type = NED_TOKEN_TYPE_CURSOR_LEFT;
                    handle_token(ctx, &token);
                } else if ( byte == 'H' ) {
                    token.type = NED_TOKEN_TYPE_HOME;
                    handle_token(ctx, &token);
                } else if ( byte == 'F' ) {
                    token.type = NED_TOKEN_TYPE_END;
                    handle_token(ctx, &token);
                } else if ( byte == 'R' ) {
                    token.type = NED_TOKEN_TYPE_CURSOR_POSITION;
                    token.data.position.row = (uint16_t) ctx->lex_arg[0];
                    token.data.position.col = (uint16_t) ctx->lex_arg[1];
                    handle_token(ctx, &token);
                } else if ( byte == '~' ) {
                    if ( ctx->lex_arg[0] == 1 ) {
                        token.type = NED_TOKEN_TYPE_HOME;
                        handle_token(ctx, &token);
                    } else if ( ctx->lex_arg[0] == 3 ) {
                        token.type = NED_TOKEN_TYPE_DELETE;
                        handle_token(ctx, &token);
                    } else if ( ctx->lex_arg[0] == 4 ) {
                        token.type = NED_TOKEN_TYPE_END;
                        handle_token(ctx, &token);
                    } else {
                        lex_reset(ctx);
                    }
                } else {
                    lex_reset(ctx);
                }
            } else { // ignore sequences containing intermediate characters other than ';'
                ctx->lex_state = NED_LEX_STATE_IGNORED_CSI;
            }
            break;

        case NED_LEX_STATE_IGNORED_CSI:
            if ( byte >= '@' && byte <= '~' ) { // end of ignored sequence
                lex_reset(ctx);
            }
            break;

        default:
            lex_reset(ctx);
            break;
        }
    }
}


void ned_feed(ned_t* ctx, const void* buffer, size_t buffer_size) {
    NED_ASSERT_CTX(ctx);

    const uint8_t* buffer_i = buffer;
    const uint8_t* buffer_e = buffer_i + buffer_size;
    ned_token_t event;

    while(buffer_i < buffer_e) {
        lex_step(ctx, *buffer_i++);
    }
}


void ned_redraw(ned_t* ctx) {
    NED_ASSERT_CTX(ctx);

}


ned_result_t ned_read_line(ned_t* ctx, const char* prompt, ned_read_callback_t read_callback, void* user_data) {
    NED_ASSERT_CTX(ctx);
    NED_ASSERT(read_callback);

    ned_result_t result;
    size_t prompt_len;

    // check read operation is not already in progress
    if ( ctx->read_callback ) {
        return NED_RESULT_BUSY;
    }

    // persist prompt in input buffer
    prompt_len = prompt ? strlen(prompt) : 0;

    if ( prompt_len > ctx->input_buffer_size ) {
        return NED_RESULT_PROMPT_TOO_LONG;
    }

    memcpy(ctx->input_buffer, prompt, prompt_len);
    ctx->input_buffer_start = prompt_len;
    ctx->input_buffer_idx = prompt_len;

    // send initial cursor position request to begin editing session
    result == request_cursor_position(ctx);

    if ( result != NED_RESULT_OK ) {
        return result;
    }

    // set up read operation callback
    ctx->read_callback = read_callback;
    ctx->read_user_data = user_data;

    return NED_RESULT_OK;
}


void ned_init(ned_t* ctx, void* buffer, size_t buffer_size, ned_write_callback_t write_callback, void* user_data) {
    NED_ASSERT(ctx);
    NED_ASSERT(buffer);
    NED_ASSERT(buffer_size >= NED_MIN_BUFFER_SIZE);
    NED_ASSERT(write_callback);

    ctx->input_buffer = buffer;
    ctx->input_buffer_size = buffer_size;
    ctx->input_buffer_idx = 0;
    ctx->output_buffer_idx = 0;
    ctx->write_callback = write_callback;
    ctx->write_user_data = user_data;
    ctx->terminal_width = 80;
    ctx->cursor_col = 0;
    ctx->cursor_row = 0;
    ctx->read_callback = NULL;
    ctx->read_user_data = NULL;

    lex_reset(ctx);

#ifndef NED_NO_ASSERT
    ctx->tag = NED_TAG;
#endif // NED_NO_ASSERT
}
