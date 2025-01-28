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

#include <arpa/inet.h>
#include <netdb.h>

#include "app-assert.h"
#include "app-net-utils.h"


app_result_t app_net_utils_str_to_addr(struct sockaddr_in* addr, const char* str, const char* default_address) {
    APP_ASSERT(addr);
    APP_ASSERT(str);

    const char *address = NULL;
    char *port_str = NULL;
    char str_copy[256];

    // copy input string to avoid modifying the original
    strncpy(str_copy, str, sizeof(str_copy) - 1);
    str_copy[sizeof(str_copy) - 1] = '\0';

    // split into address and port using ':' as delimiter
    port_str = strrchr(str_copy, ':');

    if (port_str) {
        *port_str = '\0';
        port_str++;
        address = str_copy;
    } else {
        port_str = str_copy;

        if ( default_address ) {
            address = default_address;
        } else {
            return APP_RESULT_INVALID_ARG;
        }
    }

    // convert the port string to an integer
    char *endptr;
    int port = strtol(port_str, &endptr, 10);

    if (*endptr != '\0' || port < 1 || port > 65535) {
        return APP_RESULT_INVALID_ARG;
    }

    // resolve the address (handle hostname or IP address)
    struct in_addr ip_addr;

    if (inet_pton(AF_INET, address, &ip_addr) <= 0) {
        struct hostent *host = gethostbyname(address);
        if (!host) {
            return APP_RESULT_INVALID_ARG;
        }

        ip_addr = *(struct in_addr *)host->h_addr;
    }

    // initialize the sockaddr_in structure
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr = ip_addr;

    return APP_RESULT_OK;
}
