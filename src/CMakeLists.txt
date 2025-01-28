# x0 - A lightweight RISC-V (RV32IM) simulator with GDB and Lua integration.
#
# Copyright (C) 2024-2025 Matthew T. Bucknall <matthew.bucknall@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
# documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
# OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# define 'x0' executable
add_executable(x0
        app-event.c
        app-heap.c
        app-log.c
        app-loop.c
        app-net-utils.c
        app-options.c
        main.c
)

# dependencies
target_link_libraries(x0 PRIVATE libtelnet linenoise lua)

# set optimization level depending on build type
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
target_compile_options(x0 PRIVATE -Og -g)
else()
target_compile_options(x0 PRIVATE -O3)
target_compile_definitions(x0 PRIVATE NDEBUG)
endif()
