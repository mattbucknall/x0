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

#include <stdlib.h>
#include <string.h>

#include "app-heap.h"


void* app_heap_alloc(size_t size) {
    void* ptr = malloc(size);

    if ( !ptr ) {
        abort(); // no return
    }

    return ptr;
}


void* app_heap_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);

    if ( !new_ptr ) {
        abort(); // no return
    }

    return new_ptr;
}


void app_heap_free(void* ptr) {
    free(ptr);
}


char* app_heap_strdup(const char* str) {
    if ( str ) {
        size_t len = strlen(str);
        char* copy = app_heap_alloc(len + 1);

        memcpy(copy, str, len + 1);

        return copy;
    } else {
        return NULL;
    }
}
