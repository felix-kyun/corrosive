/*
    cr_log.h - v0.4.1 - Logging Library

    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/corrosive

    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob

    For other informations, see the end of this file.
 */

#ifndef CR_LOG_H
#define CR_LOG_H

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CR_LOG_LEVEL_TRACE 0
#define CR_LOG_LEVEL_DEBUG 1
#define CR_LOG_LEVEL_INFO  2
#define CR_LOG_LEVEL_WARN  3
#define CR_LOG_LEVEL_ERROR 4
#define CR_LOG_LEVEL_FATAL 5
#define CR_LOG_LEVEL_OFF   6

// CR_LOG_SINK_LIMIT
#ifndef CR_LOG_SINK_LIMIT
#define CR_LOG_SINK_LIMIT 8
#endif

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

// sinks {{{
#ifndef CR_LOG_SINK_FILE_BUFFER
#define CR_LOG_SINK_FILE_BUFFER 10240
#endif

typedef struct cr_log_sink_meta_t {
    uint8_t         level;
    struct timespec time_data;
    const char     *filename;
    int             line;
    const char     *function;
} cr_log_sink_meta_t;

typedef struct cr_log_sink_type_t {
    void (*sink_process)(const char *msg, const cr_log_sink_meta_t *meta, void *sink_state);
    void (*sink_flush)(void *sink_state);
    void (*sink_free)(void *sink_state);
} cr_log_sink_type_t;

typedef struct cr_log_sink_t {
    cr_log_sink_type_t type;
    void              *state;
} cr_log_sink_t;

void cr_log_sink_add(cr_log_sink_t sink);
// cr_log_sink_t cr_log_sink_console();

// file sink
struct cr_log_sink_file_config_t {
    const char *target;
    bool        truncate;
};

#define cr_log_sink_file(...) cr_log_sink_file_new((struct cr_log_sink_file_config_t) { __VA_ARGS__ })
cr_log_sink_t cr_log_sink_file_new(struct cr_log_sink_file_config_t);
// }}}

typedef uint8_t log_level_t;
struct cr_log_state {
    log_level_t   level;
    cr_log_sink_t sinks[CR_LOG_SINK_LIMIT];
    size_t        sink_count;
    char         *buffer;
};

void cr_log_init(int *argc, char ***argv);
void cr_log_set_level(log_level_t level);
void cr_log_flush();
void cr_log_free();

[[gnu::format(__printf__, 5, 6)]]
void cr_log(log_level_t level, const char *file, int line, const char *func, const char *fmt, ...);

#if defined(CR_LOG_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

#include <stdarg.h>
#include <sys/time.h>

// clang-format off
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
// clang-format on

static struct cr_log_state logger_state = { 0 };

void
cr_log_init(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    logger_state.level  = CR_LOG_LEVEL_INFO;
    logger_state.buffer = (char *)malloc(CR_LOG_BUFFER_SIZE);
}

void
cr_log_set_level(log_level_t level)
{
    logger_state.level = level;
}

void
cr_log_flush()
{
    for (int i = 0; i < (int)logger_state.sink_count; i++) {
        logger_state.sinks[i].type.sink_flush(logger_state.sinks[i].state);
    }
}

void
cr_log_free()
{
    for (int i = 0; i < (int)logger_state.sink_count; i++) {
        logger_state.sinks[i].type.sink_free(logger_state.sinks[i].state);
    }
    free(logger_state.buffer);
}

void
cr_log(log_level_t level, const char *file, int line, const char *func, const char *fmt, ...)
{
    // runtime purge
    if (level < logger_state.level) {
        return;
    }

    cr_log_sink_meta_t meta = {
        .level     = level,
        .time_data = { 0 },
        .filename  = file,
        .line      = line,
        .function  = func,
    };

    clock_gettime(CLOCK_REALTIME_COARSE, &meta.time_data);

    va_list args;
    va_start(args, fmt);
    (void)vsnprintf(logger_state.buffer, CR_LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    for (int i = 0; i < (int)logger_state.sink_count; i++) {
        logger_state.sinks[i].type.sink_process(logger_state.buffer, &meta, logger_state.sinks[i].state);
    }
}

// sinks {{{

void
cr_log_sink_add(cr_log_sink_t sink)
{
    if (logger_state.sink_count < CR_LOG_SINK_LIMIT) {
        logger_state.sinks[logger_state.sink_count++] = sink;
    }
}

// file sink
typedef struct cr_log_sink_file_state_t {
    FILE *file_stream;
    char *buffer;
    int   offset;
} cr_log_sink_file_state_t;

void
cr_log_sink_file_flush(void *sink_state)
{
    cr_log_sink_file_state_t *state = sink_state;

    // write buffer to file and flush
    (void)fwrite(state->buffer, 1, (size_t)state->offset, state->file_stream);
    (void)fflush(state->file_stream);
    state->offset = 0;
}

void
cr_log_sink_file_process(const char *msg, const cr_log_sink_meta_t *meta, void *sink_state)
{
    char                      buf[256];
    size_t                    buf_offset = 0;
    cr_log_sink_file_state_t *state      = sink_state;

    // time
    struct tm *time_info = localtime(&meta->time_data.tv_sec);
    buf_offset += strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S", time_info);
    buf_offset
        += (size_t)snprintf(buf + buf_offset, sizeof(buf) - buf_offset, ".%03ld] ", meta->time_data.tv_nsec / 1000000);

    // level
    buf_offset
        += (size_t)snprintf(buf + buf_offset, sizeof(buf) - buf_offset, "[%s] ", cr_log_level_names[meta->level]);

    // location
    buf_offset += (size_t)snprintf(
        buf + buf_offset, sizeof(buf) - buf_offset, "[%s:%d %s] ", meta->filename, meta->line, meta->function);

    // try
    int offset
        = snprintf(state->buffer + state->offset, CR_LOG_SINK_FILE_BUFFER - (size_t)state->offset, "%s%s\n", buf, msg);

    if (offset < 0) {
        perror("(snprintf) failed to append to file sink buffer");
        exit(EXIT_FAILURE);
    }

    if (offset > CR_LOG_SINK_FILE_BUFFER) {
        // larger than buffer, write directly
        cr_log_sink_file_flush(sink_state);
        (void)fprintf(state->file_stream, "%s", msg);
        return;
    }

    if (state->offset + offset >= CR_LOG_SINK_FILE_BUFFER) {
        // buffer overflow, flush and retry
        cr_log_sink_file_flush(sink_state);
        int ret = snprintf(
            state->buffer + state->offset, CR_LOG_SINK_FILE_BUFFER - (size_t)state->offset, "%s%s\n", buf, msg);

        if (ret < 0) {
            perror("(snprintf) failed to append to file sink buffer");
            exit(EXIT_FAILURE);
        }
        state->offset += ret;
    }

    state->offset += offset;
}

void
cr_log_sink_file_free(void *sink_state)
{
    struct cr_log_sink_file_state_t *state = sink_state;

    cr_log_sink_file_flush(state);
    (void)fclose(state->file_stream);

    free(state->buffer);
    free(state);
}

cr_log_sink_t
cr_log_sink_file_new(struct cr_log_sink_file_config_t config)
{
    cr_log_sink_file_state_t *state = malloc(sizeof(cr_log_sink_file_state_t));
    if (!state) {
        perror("(malloc) file sink state allocation failed");
        exit(EXIT_FAILURE);
    }

    state->buffer = (char *)malloc(CR_LOG_SINK_FILE_BUFFER);
    if (!state->buffer) {
        perror("(malloc) file sink buffer allocation failed");
        exit(EXIT_FAILURE);
    }

    if (config.truncate) {
        state->file_stream = fopen(config.target, "w");
    } else {
        state->file_stream = fopen(config.target, "a");
    }

    if (!state->file_stream) {
        perror("(fopen) file sink open failed");
        exit(EXIT_FAILURE);
    }

    state->offset = 0;

    return (cr_log_sink_t)
    {
        .type = {
            .sink_process = cr_log_sink_file_process,
            .sink_flush   = cr_log_sink_file_flush,
            .sink_free    = cr_log_sink_file_free,
        },
        .state = state,
    };
}

// }}}

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
