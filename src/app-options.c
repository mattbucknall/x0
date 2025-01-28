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
#include <stdnoreturn.h>

#include "app-assert.h"
#include "app-heap.h"
#include "app-net-utils.h"
#include "app-options.h"
#include "app-version.h"


#define APP_OPTIONS_DEFAULT_GDB_BIND_ADDRESS        "127.0.0.1:3333"
#define APP_OPTIONS_DEFAULT_LUA_BIND_ADDRESS        "127.0.0.1:2323"
#define APP_OPTIONS_DEFAULT_MACH_BIND_ADDRESS       "127.0.0.1:4242"
#define APP_OPTIONS_DEFAULT_ROM_SIZE                (4 * 1024 * 1024)
#define APP_OPTIONS_MAX_ROM_SIZE                    (256 * 1024 * 1024)
#define APP_OPTIONS_DEFAULT_RAM_SIZE                (4 * 1024 * 1024)
#define APP_OPTIONS_MAX_RAM_SIZE                    (256 * 1024 * 1024)


static const char* m_exec_name;
static app_options_lua_input_t* m_lua_input_head;
static app_options_lua_input_t* m_lua_input_tail;
static struct sockaddr_in m_gdb_bind_address;
static struct sockaddr_in m_lua_bind_address;
static struct sockaddr_in m_mach_bind_address;
static app_log_priority_t m_min_log_priority;
static uint32_t m_rom_size;
static uint32_t m_ram_size;
static bool m_testing_enabled;
static const char* m_elf_path;


app_options_lua_input_t* app_options_lua_input(void) {
    return m_lua_input_head;
}


const struct sockaddr_in* app_options_gdb_bind_address(void) {
    return &m_gdb_bind_address;
}


const struct sockaddr_in* app_options_lua_bind_address(void) {
    return &m_lua_bind_address;
}


const struct sockaddr_in* app_options_mach_bind_address(void) {
    return &m_mach_bind_address;
}


app_log_priority_t app_options_min_log_priority(void) {
    return m_min_log_priority;
}


uint32_t app_options_rom_size(void) {
    return m_rom_size;
}


uint32_t app_options_ram_size(void) {
    return m_ram_size;
}


bool app_options_testing_enabled(void) {
    return m_testing_enabled;
}


const char* app_options_elf_path(void) {
    return m_elf_path;
}


static noreturn void print_help_info(void) {
    printf("Usage: %s [OPTIONS...] <ELF-PATH>\n", m_exec_name);

    puts("  -c <COMMAND>           Execute Lua command before starting core.");
    puts("  -f <PATH>              Execute Lua script before starting core.");
    puts("  -g <[ADDRESS:]PORT>    Bind remote GDB service to specified address and port");
    puts("                         (default: " APP_OPTIONS_DEFAULT_GDB_BIND_ADDRESS ").");
    puts("  -l <[ADDRESS:]PORT>    Bind Lua telnet service to specified address and port");
    puts("                         (default: " APP_OPTIONS_DEFAULT_LUA_BIND_ADDRESS ").");
    puts("  -m <[ADDRESS:]PORT>    Bind machine interface service to specified address and port");
    puts("                         (default: " APP_OPTIONS_DEFAULT_MACH_BIND_ADDRESS ").");
    puts("  -q                     Quiet log output (only log errors).");
    puts("  -r <SIZE>              Set ROM size in bytes (must be multiple of 4,");
    printf("                         default size = %u, max = %u).\n", APP_OPTIONS_DEFAULT_ROM_SIZE,
            APP_OPTIONS_MAX_ROM_SIZE);
    puts("  -a <SIZE>              Set RAM size in bytes (must be multiple of 4,");
    printf("                         default size = %u, max = %u).\n", APP_OPTIONS_DEFAULT_RAM_SIZE,
            APP_OPTIONS_MAX_RAM_SIZE);
    puts("  -t                     Enable custom test instructions.");
    puts("  -h, -?                 Print this help info and terminate.");
    puts("  -v                     Print version info and terminate.");
    puts("  -V                     Verbose log output (includes debugging messages).");

    exit(EXIT_SUCCESS);
}


static noreturn void print_bad_arg_advice(void) {
    fprintf(stderr, "Try '%s -?' for more information.\n", m_exec_name);
    exit(EXIT_FAILURE);
}


static noreturn void print_version_info(void) {
    puts(APP_VERSION_STR);
    exit(EXIT_SUCCESS);
}


static void append_lua_input(app_options_lua_input_type_t type, const char* data) {
    // allocate lua input descriptor
    app_options_lua_input_t* input = app_heap_alloc(sizeof(app_options_lua_input_t));

    // initialise descriptor
    input->next = NULL;
    input->type = type;
    input->data = data;

    // append descriptor to list
    if ( m_lua_input_tail ) {
        m_lua_input_tail->next = input;
    } else {
        m_lua_input_head = input;
    }

    m_lua_input_tail = input;
}


static const char* get_exec_name(const char* path) {
    const char* name = path;

    while (*path) {
        if ( *path++ == '/' ) {
            name = path;
        }
    }

    return name;
}


static char get_flag(const char* arg) {
    APP_ASSERT(arg);

    if ( arg[0] == '-' && arg[1] > ' ' && arg[2] == 0 ) {
        return arg[1];
    } else {
        return 0;
    }
}


static void check_for_operand(char flag, const char* operand_name, int i, int argc) {
    // exit with error message if next argument in array does not exist
    if ( (i + 1) >= argc ) {
        fprintf(stderr, "Option -%c requires %s operand\n", flag, operand_name);
        print_bad_arg_advice(); // no return
    }
}


static void parse_bind_address_operand(char flag, struct sockaddr_in* dest, const char* arg) {
    APP_ASSERT(dest);
    APP_ASSERT(arg);

    if ( app_net_utils_str_to_addr(dest, arg, "127.0.0.1") != APP_RESULT_OK ) {
        fprintf(stderr, "-%c: Invalid address:port\n", flag);
        print_bad_arg_advice(); // no return
    }
}


static void parse_mem_size_operand(char flag, uint32_t* dest, long max, const char* arg) {
    APP_ASSERT(dest);
    APP_ASSERT(max > 0);
    APP_ASSERT(arg);

    char* endptr;
    long size = strtol(arg, &endptr, 0);

    if ( *endptr != '\0' || size < 1 || size > max || (size & 3) ) {
        fprintf(stderr, "-%c: Invalid size\n", flag);
        print_bad_arg_advice(); // no return
    }

    *dest = (uint32_t) size;
}


static void cleanup(void) {
    // destroy Lua input descriptors
    while(m_lua_input_head) {
        app_options_lua_input_t* next = m_lua_input_head->next;
        app_heap_free(m_lua_input_head);
        m_lua_input_head = next;
    }

    m_lua_input_tail = NULL;
}


void app_options_init(int argc, char* argv[]) {
    APP_ASSERT(argc > 0);
    APP_ASSERT(argv);

    // register cleanup function
    if ( atexit(cleanup) < 0 ) {
        abort(); // no return
    }

    // set executable name
    m_exec_name = get_exec_name(argv[0]);

    // set default option values
    app_net_utils_str_to_addr(&m_gdb_bind_address, APP_OPTIONS_DEFAULT_GDB_BIND_ADDRESS, NULL);
    app_net_utils_str_to_addr(&m_lua_bind_address, APP_OPTIONS_DEFAULT_LUA_BIND_ADDRESS, NULL);

    m_min_log_priority = APP_LOG_PRIORITY_INFO;
    m_rom_size = APP_OPTIONS_DEFAULT_ROM_SIZE;
    m_ram_size = APP_OPTIONS_DEFAULT_RAM_SIZE;
    m_testing_enabled = false;

    // process terminating flags first
    for (int i = 1; i < argc; ++i) {
        switch(get_flag(argv[i])) {
        case 'h':
        case '?':
            // display help info and terminate
            print_help_info(); // no return

        case 'v':
            // display version info and terminate
            print_version_info(); // no return

        default:
            // ignore other flags/arguments
            break;
        }
    }

    // process non-terminating flags
    for (int i = 1; i < argc; ++i) {
        char flag = get_flag(argv[i]);

        switch(flag) {
        case 'c':
            check_for_operand(flag, "<COMMAND>", i, argc); // may exit
            append_lua_input(APP_OPTIONS_LUA_INPUT_TYPE_CHUNK, argv[++i]);
            break;

        case 'f':
            check_for_operand(flag, "<PATH>", i, argc); // may exit
            append_lua_input(APP_OPTIONS_LUA_INPUT_TYPE_FILE, argv[++i]);
            break;

        case 'g':
            check_for_operand(flag, "<[ADDRESS:]PORT>", i, argc); // may exit
            parse_bind_address_operand(flag, &m_gdb_bind_address, argv[++i]); // may exit
            break;

        case 'l':
            check_for_operand(flag, "<[ADDRESS:]PORT>", i, argc); // may exit
            parse_bind_address_operand(flag, &m_lua_bind_address, argv[++i]); // may exit
            break;

        case 'm':
            check_for_operand(flag, "<[ADDRESS:]PORT>", i, argc); // may exit
            parse_bind_address_operand(flag, &m_mach_bind_address, argv[++i]); // may exit
            break;

        case 'q':
            // quiet mode - set min log priority to errors or worse
            m_min_log_priority = APP_LOG_PRIORITY_ERROR;
            break;

        case 'r':
            check_for_operand(flag, "<SIZE>", i, argc); // may exit
            parse_mem_size_operand(flag, &m_rom_size, APP_OPTIONS_MAX_ROM_SIZE, argv[++i]); // may exit
            break;

        case 'a':
            check_for_operand(flag, "<SIZE>", i, argc); // may exit
            parse_mem_size_operand(flag, &m_ram_size, APP_OPTIONS_MAX_RAM_SIZE, argv[++i]); // may exit
            break;

        case 't':
            // enable test mode
            m_testing_enabled = true;
            break;

        case 'V':
            // verbose mode - set min log priority to detail
            m_min_log_priority = APP_LOG_PRIORITY_DETAIL;
            break;

        default:
            if ( flag ) {
                fprintf(stderr, "Invalid option -%c\n", flag);
                print_bad_arg_advice(); // no return
            } else if ( m_elf_path ) {
                fprintf(stderr, "<ELF-PATH> already specified\n");
                print_bad_arg_advice(); // no return
            } else {
                m_elf_path = argv[i];
            }
        }
    }

    // check ELF path has been specified
    if ( !m_elf_path ) {
        fprintf(stderr, "<ELF-PATH> not specified\n");
        print_bad_arg_advice(); // no return
    }
}
