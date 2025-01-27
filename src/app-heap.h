/// @file app-heap.h

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

#include <stddef.h>


/**
 * Allocates space on heap.
 *
 * @param size      Size of allocation in bytes.
 *
 * @return  Pointer to allocated memory (never NULL).
 * 
 * @note    Performs abort if allocation fails.
 */
void* app_heap_alloc(size_t size);


/**
 * Reallocates space on heap for previously allocated block of memory. Contents of original allocation will be copied
 * to new, truncated if new allocation is smaller than the original.
 * 
 * @param ptr   Pointer to previously allocated memory (can also be NULL, which case function acts like app_heap_alloc).
 * 
 * @param size  New size of allocation in bytes. Zero causes function to act the same as app_heap_free.
 * 
 * @return  Pointer to reallocator memory (never NULL).
 * 
 * @note    Performs abort if reallocation fails.
 */
void* app_heap_realloc(void* ptr, size_t size);


/**
 * Frees memory allocated with app_alloc.
 *
 * @param ptr   Pointer to allocated memory to free.
 *
 * @note    Does nothing if ptr is NULL.
 */
void app_heap_free(void* ptr);
