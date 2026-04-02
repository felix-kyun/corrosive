/*
    cr_log.h - v0.2.0 - Logging Library

    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/shl

    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob

    For other informations, see the end of this file.
 */

#ifndef CR_LOG_H
#define CR_LOG_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CR_LOG_LEVEL_TRACE 0
#define CR_LOG_LEVEL_DEBUG 1
#define CR_LOG_LEVEL_INFO  2
#define CR_LOG_LEVEL_WARN  3
#define CR_LOG_LEVEL_ERROR 4
#define CR_LOG_LEVEL_FATAL 5
#define CR_LOG_LEVEL_OFF   6

// CR_LOG_BUFFER_SIZE
#ifndef CR_LOG_BUFFER_SIZE
#define CR_LOG_BUFFER_SIZE 2048
#endif

// CR_LOG_PURGE_LEVEL
#ifndef CR_LOG_PURGE_LEVEL
#define CR_LOG_PURGE_LEVEL CR_LOG_LEVEL_TRACE
#endif

#define CR_LOG(level, ...) cr_log(level, __FILE__, __LINE__, __func__, __VA_ARGS__)

// CR_LOG_TRACE_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_TRACE
#define cr_log_trace(...) CR_LOG(CR_LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define cr_log_trace(...) ((void)0)
#endif

// CR_LOG_DEBUG_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_DEBUG
#define cr_log_debug(...) CR_LOG(CR_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define cr_log_debug(...) ((void)0)
#endif

// CR_LOG_INFO_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_INFO
#define cr_log_info(...) CR_LOG(CR_LOG_LEVEL_INFO, __VA_ARGS__)
#else
#define cr_log_info(...) ((void)0)
#endif

// CR_LOG_WARN_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_WARN
#define cr_log_warn(...) CR_LOG(CR_LOG_LEVEL_WARN, __VA_ARGS__)
#else
#define cr_log_warn(...) ((void)0)
#endif

// CR_LOG_ERROR_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_ERROR
#define cr_log_error(...) CR_LOG(CR_LOG_LEVEL_ERROR, __VA_ARGS__)
#else
#define cr_log_error(...) ((void)0)
#endif

// CR_LOG_FATAL_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_FATAL
#define cr_log_fatal(...)                                                                                              \
    do {                                                                                                               \
        CR_LOG(CR_LOG_LEVEL_FATAL, __VA_ARGS__);                                                                       \
        abort();                                                                                                       \
    } while (0)
#else
#define cr_log_fatal(...) abort()
#endif

typedef uint8_t log_level_t;
struct cr_log_state {
    log_level_t level;
    FILE*       stream;
    char*       buffer;
};

void cr_log_init(int* argc, char*** argv);
void cr_log_set_level(log_level_t level);
void cr_log_set_stream(FILE* stream);
void cr_log_free();

[[gnu::format(__printf__, 5, 6)]]
void cr_log(log_level_t level, const char* file, int line, const char* func, const char* fmt, ...);

#if defined(CR_LOG_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

static const char* cr_log_reset    = "\x1b[0m";
static const char* cr_log_colors[] = {
    [CR_LOG_LEVEL_TRACE] = "\x1b[34m", // blue
    [CR_LOG_LEVEL_DEBUG] = "\x1b[36m", // cyan
    [CR_LOG_LEVEL_INFO]  = "\x1b[32m", // green
    [CR_LOG_LEVEL_WARN]  = "\x1b[33m", // yellow
    [CR_LOG_LEVEL_ERROR] = "\x1b[31m", // red
    [CR_LOG_LEVEL_FATAL] = "\x1b[35m", // magenta
};
static const char* cr_log_level_names[] = {
    [CR_LOG_LEVEL_TRACE] = "TRACE",
    [CR_LOG_LEVEL_DEBUG] = "DEBUG",
    [CR_LOG_LEVEL_INFO]  = "INFO ",
    [CR_LOG_LEVEL_WARN]  = "WARN ",
    [CR_LOG_LEVEL_ERROR] = "ERROR",
    [CR_LOG_LEVEL_FATAL] = "FATAL",
};

static struct cr_log_state state = { 0 };

void
cr_log_init(int* argc, char*** argv)
{
    (void)argc;
    (void)argv;

    state.level  = CR_LOG_LEVEL_INFO;
    state.stream = stderr;
    state.buffer = (char*)malloc(CR_LOG_BUFFER_SIZE);
}

void
cr_log_set_level(log_level_t level)
{
    state.level = level;
}

void
cr_log_set_stream(FILE* stream)
{
    state.stream = stream;
}

void
cr_log_free()
{
    free(state.buffer);
}

void
cr_log(log_level_t level, [[maybe_unused]] const char* file, [[maybe_unused]] int line,
    [[maybe_unused]] const char* func, const char* fmt, ...)
{
    // runtime purge
    if (level < state.level) {
        return;
    }

    size_t offset = 0;

// timestamp
#ifndef CR_LOG_DISABLE_TIMESTAMP
    struct timeval timeval;
    gettimeofday(&timeval, NULL);
    struct tm* tm_info = localtime(&timeval.tv_sec);
    offset += strftime(state.buffer + offset, CR_LOG_BUFFER_SIZE - offset, "[%Y-%m-%d %H:%M:%S", tm_info);
    offset += (size_t)snprintf(state.buffer + offset, CR_LOG_BUFFER_SIZE - offset, ".%03ld] ", timeval.tv_usec);
#endif

    // level
    offset += (size_t)snprintf(state.buffer + offset, CR_LOG_BUFFER_SIZE - offset, "[%s%s%s] ", cr_log_colors[level],
        cr_log_level_names[level], cr_log_reset);

// location
#ifndef CR_LOG_DISABLE_LOCATION
    offset += (size_t)snprintf(state.buffer + offset, CR_LOG_BUFFER_SIZE - offset, "[%s:%d %s] ", file, line, func);
#endif

    va_list args;
    va_start(args, fmt);
    offset += (size_t)vsnprintf(state.buffer + offset, CR_LOG_BUFFER_SIZE - offset, fmt, args);
    va_end(args);

    fprintf(state.stream, "%s\n", state.buffer);
    fflush(state.stream);
}

#endif // CR_LOG_IMPL
#endif // CR_LOG_H

/*
This is a single header library that provides logging functions.
This is part of the Corrosive library.

To use this library, do this in *one* of your source files:
    #define CR_LOG_IMPL
    #include "cr_log.h"

Table Of Contents
    Compile time options
    Documentation
    Examples
    License
    Credits

Compile time options
        To be added.

Documentation
        To be added.

Examples
        To be added.

MIT License
    Copyright (c) 2026 Praise Jacob <iampraisejacob@gmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

Credits
    Praise Jacob 	library API/implementation
    Sean Barret 	built STB which inspired this library
 */
