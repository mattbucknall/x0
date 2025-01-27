/// @file app-options.h

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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <netinet/in.h>

#include "app-log.h"


/**
 * Enumeration of Lua input type IDs.
 */
typedef enum {
    APP_OPTIONS_LUA_INPUT_TYPE_FILE,    ///< Input data is a path to a Lua script file.
    APP_OPTIONS_LUA_INPUT_TYPE_CHUNK    ///< Input data is a Lua chunk.
} app_options_lua_input_type_t;


/**
 * Lua input descriptor.
 */
typedef struct app_options_lua_input {
    struct app_options_lua_input* next; ///< Pointer to next input descriptor or NULL if descriptor is last in list.
    app_options_lua_input_type_t type;  ///< Input data type.
    const char* data;                   ///< Input ptor data (not owned by descriptor).
} app_options_lua_input_t;


/**
 * Parses command-line arguments and sets runtime options.
 *
 * @param argc      Number of arguments in argv.
 * @param argv      Array of command-line arguments.
 *
 * @note May terminate if command-line arguments are invalid or if help/version flags are present.
 */
void app_options_init(int argc, char* argv[]);


/**
 * @return  Head of Lua input descriptor linked list. NULL if list is empty.
 */
app_options_lua_input_t* app_options_lua_input(void);


/**
 * @return  Bind address for GDB service.
 */
const struct sockaddr_in* app_options_gdb_bind_address(void);


/**
 * @return  Bind address for Lua telnet service.
 */
const struct sockaddr_in* app_options_lua_bind_address(void);


/**
 * @return  Bind address for machine interface service.
 */
const struct sockaddr_in* app_options_mach_bind_address(void);


/**
 * @return  Minimum log priority.
 */
app_log_priority_t app_options_min_log_priority(void);


/**
 * @return  ROM region size, in bytes.
 */
uint32_t app_options_rom_size(void);


/**
 * @return  RAM region size, in bytes.
 */
uint32_t app_options_ram_size(void);


/**
 * @return  true if testing enabled.
 */
bool app_options_testing_enabled(void);


/**
 * @return  ELF file path.
 */
const char* app_options_elf_path(void);
