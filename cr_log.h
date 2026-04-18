/*
    cr_log.h - v0.6.1 - Logging Library

    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/corrosive

    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob

    For other informations, see the end of this file.
 */

#ifndef CR_LOG_H
#define CR_LOG_H

#define _POSIX_C_SOURCE 202405L
#include <stdint.h>
#include <stdio.h>

#define CR_LOG_LEVEL_TRACE 0
#define CR_LOG_LEVEL_DEBUG 1
#define CR_LOG_LEVEL_INFO  2
#define CR_LOG_LEVEL_WARN  3
#define CR_LOG_LEVEL_ERROR 4
#define CR_LOG_LEVEL_FATAL 5
#define CR_LOG_LEVEL_OFF   6

#ifndef CR_LOG_SINK_LIMIT
#define CR_LOG_SINK_LIMIT 8
#endif

#ifndef CR_LOG_BUFFER_SIZE
#define CR_LOG_BUFFER_SIZE 2048
#endif

#ifndef CR_LOG_PURGE_LEVEL
#define CR_LOG_PURGE_LEVEL CR_LOG_LEVEL_TRACE
#endif

#ifndef CR_LOG_SINK_FILE_BUFFER
#define CR_LOG_SINK_FILE_BUFFER 10240
#endif

#ifndef CR_LOG_SCOPE_MAX_SIZE
#define CR_LOG_SCOPE_MAX_SIZE 16
#endif

#ifndef CR_LOG_SCOPE_BUFFER
#define CR_LOG_SCOPE_BUFFER 256
#endif

#ifndef CR_LOG_QUEUE_SIZE_POWER
#define CR_LOG_QUEUE_SIZE_POWER 12
#endif

#ifndef CR_LOG_QUEUE_ITEM_SIZE
#define CR_LOG_QUEUE_ITEM_SIZE 512
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS
#define CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS 128
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF
#define CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF 256
#endif

#define CACHE_LINE_SIZE 64

// * compile time purging

#define CR_LOG(level, ...) cr_log(level, __FILE__, __LINE__, __func__, __VA_ARGS__)

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_TRACE
#define cr_log_trace(fmt, ...) CR_LOG(CR_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
#define cr_log_trace(...) ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_DEBUG
#define cr_log_debug(fmt, ...) CR_LOG(CR_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define cr_log_debug(...) ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_INFO
#define cr_log_info(fmt, ...) CR_LOG(CR_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define cr_log_info(...) ((void)0)
#endif

// CR_LOG_WARN_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_WARN
#define cr_log_warn(fmt, ...) CR_LOG(CR_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#else
#define cr_log_warn(...) ((void)0)
#endif

// CR_LOG_ERROR_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_ERROR
#define cr_log_error(fmt, ...) CR_LOG(CR_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
#define cr_log_error(...) ((void)0)
#endif

// CR_LOG_FATAL_ENABLED
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_FATAL
#define cr_log_fatal(fmt, ...) CR_LOG(CR_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#else
#define cr_log_fatal(...) ((void)0)
#endif

typedef uint8_t cr_log_level_t;

void cr_log_init(void);
void cr_log_flush(void);
void cr_log_free(void);

[[gnu::hot, gnu::format(__printf__, 5, 6)]]
void cr_log(cr_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...);
void cr_log_set_level(cr_log_level_t level);

// * scope
_Thread_local struct {
    int  depth;
    char buffer[CR_LOG_SCOPE_BUFFER];
    int  buffer_length;
    int  buffer_offsets[CR_LOG_SCOPE_MAX_SIZE];
} cr_log__scope_stack = { 0 };

void cr_log_scope_push(const char *scope);
void cr_log_scope_pop(void);

typedef struct cr_log_item_t cr_log_item_t;

// * sinks
typedef struct cr_log_sink_t {
    void (*process)(void *sink_state, const cr_log_item_t *item);
    void (*flush)(void *sink_state);
    void (*free)(void *sink_state);
    void *state;
} cr_log_sink_t;

void cr_log_sink_add(cr_log_sink_t sink);

// ** file sink
typedef struct cr_log_sink_file_config_t {
    const char    *target;
    const FILE    *file;
    bool           truncate;
    bool           disable_close;
    bool           color;
    cr_log_level_t level;

    // -1 to disable, 0 to use default, >0 to set custom buffer size
    ssize_t ibuffer_size;

    int (*formatter)(
        char *buf, size_t size, const cr_log_item_t *event, const struct cr_log_sink_file_config_t *config);
} cr_log_sink_file_config_t;

#define cr_log_sink_file(...) cr_log_sink_file_new((struct cr_log_sink_file_config_t) { __VA_ARGS__ })
#define cr_log_sink_default()                                                                                          \
    cr_log_sink_file_new((struct cr_log_sink_file_config_t) {                                                          \
        .file = stderr, .color = true, .ibuffer_size = -1, .disable_close = true })

cr_log_sink_t cr_log_sink_file_new(struct cr_log_sink_file_config_t config);

#if defined(CR_LOG_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

#include <immintrin.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define err(fmt, ...)                                                                                                  \
    do {                                                                                                               \
        (void)fprintf(stderr, "error(%s:%s:%d)", __FILE__, __func__, __LINE__);                                        \
        (void)fprintf(stderr, (fmt), ##__VA_ARGS__);                                                                   \
    } while (0)

#define atomic_load_relaxed(target)         atomic_load_explicit(target, memory_order_relaxed)
#define atomic_load_acquire(target)         atomic_load_explicit(target, memory_order_acquire)
#define atomic_store_relaxed(target, value) atomic_store_explicit(target, value, memory_order_relaxed)
#define atomic_store_release(target, value) atomic_store_explicit(target, value, memory_order_release)
#define atomic_cas(target, expected, desired)                                                                          \
    atomic_compare_exchange_weak_explicit(target, expected, desired, memory_order_relaxed, memory_order_relaxed)

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

// internal api
void        queue_consumer(struct cr_log_item_t *item);
const char *cr_log__scope_get(void);

// clang-format off
static constexpr size_t buffer_size
    = CR_LOG_QUEUE_ITEM_SIZE
    - (sizeof(atomic_size_t)
    + (sizeof(uint_fast32_t) * 2)
    + sizeof(struct timespec)
    + (sizeof(const char *) * 2)
    + CR_LOG_SCOPE_MAX_SIZE);
// clang-format on

typedef struct cr_log_item_t {
    uint8_t         level;
    uint32_t        line;
    const char     *filename;
    const char     *function;
    struct timespec time;
    char            scope[CR_LOG_SCOPE_MAX_SIZE];
    char            buffer[buffer_size];
} cr_log_item_t;

struct item {
    alignas(CACHE_LINE_SIZE) atomic_size_t sequence;
    struct cr_log_item_t meta;
};

struct queue {
    alignas(CACHE_LINE_SIZE) atomic_size_t write;
    char          pad1[CACHE_LINE_SIZE - sizeof(atomic_size_t)];
    atomic_size_t read;
    char          pad2[CACHE_LINE_SIZE - sizeof(atomic_size_t)];

    size_t    mask;
    pthread_t consumer_thread;
    sem_t     items;

    struct item buffer[1 << CR_LOG_QUEUE_SIZE_POWER];
} queue;

static_assert(sizeof(struct item) == CR_LOG_QUEUE_ITEM_SIZE, "item too large");
static_assert(alignof(struct item) == CACHE_LINE_SIZE, "alignment broken");

int
enqueue_(struct cr_log_item_t *meta)
{
    for (;;) {
        size_t write_pos = atomic_load_relaxed(&queue.write);
        size_t idx       = write_pos & queue.mask;
        size_t seq       = atomic_load_relaxed(&queue.buffer[idx].sequence);

        // check availablity
        if (write_pos - seq == 0) {
            // available, try cas
            if (atomic_cas(&queue.write, &write_pos, write_pos + 1)) {
                // claimed
                queue.buffer[idx].meta = *meta;
                atomic_store_release(&queue.buffer[idx].sequence, write_pos + 1);
                sem_post(&queue.items);
                return 0;
            }
            // cas failed, retry
        } else {
            // full or stale
            return -1;
        }
    }

    return -1;
}

int
enqueue(struct cr_log_item_t meta)
{
    int backoff = 1;

    for (int i = 0; i < CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS; i++) {
        int ret = enqueue_(&meta);
        if (ret == 0) {
            return 0;
        } else if (ret == -1) {
            // spin and retry
            // exponential backoff
            for (int j = 0; j < backoff; j++) {
                _mm_pause();
            }
            backoff = (backoff < CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF) ? backoff << 1 : CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF;
        }
    }

    // drop
    return -1;
}

int
try_dequeue(void)
{
    size_t read_pos = atomic_load_relaxed(&queue.read);
    size_t idx      = read_pos & queue.mask;
    size_t seq      = atomic_load_acquire(&queue.buffer[idx].sequence);
    long   diff     = (long)seq - (long)(read_pos + 1);

    if (diff == 0) {
        // consume
        queue_consumer(&queue.buffer[idx].meta);

        // release
        atomic_store_relaxed(&queue.buffer[idx].sequence, read_pos + queue_size);
        atomic_store_relaxed(&queue.read, read_pos + 1);

        return 0;
    }

    return -1;
}

void *
dequeue([[maybe_unused]] void *arg)
{
    for (;;) {
        sem_wait(&queue.items);
        while (try_dequeue() == 0) {
            if (sem_trywait(&queue.items) != 0) {
                break;
            }
        }
    }

    return NULL;
}

static struct {
    cr_log_level_t level;
    cr_log_sink_t  sinks[CR_LOG_SINK_LIMIT];
    size_t         sink_count;
} logger_state = { 0 };

void
cr_log_init(void)
{
    logger_state.level = CR_LOG_LEVEL_INFO;
    for (size_t i = 0; i < queue_size; i++) {
        atomic_init(&queue.buffer[i].sequence, i);
    }
    queue.mask = queue_size - 1;
    atomic_init(&queue.read, 0);
    atomic_init(&queue.write, 0);
    pthread_create(&queue.consumer_thread, NULL, dequeue, NULL);
}

void
cr_log_set_level(cr_log_level_t level)
{
    logger_state.level = level;
}

void
cr_log_flush(void)
{
    for (int i = 0; i < (int)logger_state.sink_count; i++) {
        logger_state.sinks[i].flush(logger_state.sinks[i].state);
    }
}

void
cr_log_free(void)
{
    pthread_join(queue.consumer_thread, NULL);
    for (int i = 0; i < (int)logger_state.sink_count; i++) {
        logger_state.sinks[i].free(logger_state.sinks[i].state);
    }
}

void
queue_consumer(struct cr_log_item_t *item)
{
    for (int i = 0; i < (int)logger_state.sink_count; i++) {
        logger_state.sinks[i].process(logger_state.sinks[i].state, item);
    }
}

void
cr_log(cr_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...)
{
    // runtime purge
    if (level < logger_state.level) {
        return;
    }

    // clang-format off
    cr_log_item_t event = {
        // .message   = logger_state.buffer,
        .level     = level,
        .time = { 0 },
        .filename  = file,
        .line      = (uint32_t)line,
        .function  = func,
    };
    // clang-format on

    strncpy(event.scope, cr_log__scope_get(), CR_LOG_SCOPE_MAX_SIZE - 1);
    event.scope[CR_LOG_SCOPE_MAX_SIZE - 1] = '\0';

    clock_gettime(CLOCK_REALTIME_COARSE, &event.time);

    va_list args;
    va_start(args, fmt);
    (void)vsnprintf(event.buffer, buffer_size, fmt, args);
    va_end(args);

    enqueue(event);
}

// * Scope
void
cr_log__scope_push(const char *scope)
{
    auto stack = &cr_log__scope_stack;
    if (likely(stack->depth < CR_LOG_SCOPE_MAX_SIZE)) {
        // save reset point
        stack->buffer_offsets[stack->depth] = stack->buffer_length;

        int written = snprintf(
            stack->buffer + stack->buffer_length,
            (size_t)(CR_LOG_SCOPE_BUFFER - stack->buffer_length),
            "%s%s",
            stack->depth == 0 ? "" : "::",
            scope);

        if (likely(stack->buffer_length + written < CR_LOG_SCOPE_BUFFER)) {
            stack->buffer_length += written;
        } else if (unlikely(written > 0)) {
            err("(scope_push) Scope buffer overflow: %s", scope);
        } else {
            err("(scope_push) Failed to push scope %s", scope);
        }

        stack->depth++;
    }
}

void
cr_log__scope_pop(void)
{
    auto stack = &cr_log__scope_stack;
    if (stack->depth <= 0) {
        return;
    }
    stack->depth--;
    stack->buffer_length                = stack->buffer_offsets[stack->depth];
    stack->buffer[stack->buffer_length] = '\0';
}

const char *
cr_log__scope_get(void)
{
    return cr_log__scope_stack.buffer;
}

// * Sinks
void
cr_log_sink_add(cr_log_sink_t sink)
{
    if (logger_state.sink_count < CR_LOG_SINK_LIMIT) {
        logger_state.sinks[logger_state.sink_count++] = sink;
    }
}

// ** File sink
#define CR_LOG_SINK_FILE_FORMAT_MAX 256
typedef struct cr_log_sink_file_state_t {
    struct cr_log_sink_file_config_t config;
    FILE                            *file_stream;
    char                            *buffer;
    size_t                           buffer_size;
    int                              offset;
} cr_log_sink_file_state_t;

void cr_log__sink_file_flush(void *sink_state);
void cr_log__sink_file_process(void *state, const cr_log_item_t *item);
int  cr_log__sink_file_format(
    char *buffer, size_t size, const cr_log_item_t *item, const struct cr_log_sink_file_config_t *config);
void cr_log__sink_file_free(void *sink_state);

cr_log_sink_t
cr_log_sink_file_new(struct cr_log_sink_file_config_t config)
{
    cr_log_sink_file_state_t *state = malloc(sizeof(cr_log_sink_file_state_t));
    if (!state) {
        perror("(malloc) file sink state allocation failed");
        return (cr_log_sink_t) { 0 };
    }

    if (config.target != nullptr) {
        if (config.truncate) {
            state->file_stream = fopen(config.target, "w");
        } else {
            state->file_stream = fopen(config.target, "a");
        }
    } else if (config.file != nullptr) {
        state->file_stream = (FILE *)config.file;
    } else {
        err("fallback to stderr: target=%s file=%p\n", config.target, config.file);
        state->file_stream = stderr;
    }

    if (!state->file_stream) {
        perror("(fopen) file sink open failed");
        return (cr_log_sink_t) { 0 };
    }

    if (config.ibuffer_size < 0) {
        // disable buffering
        state->buffer      = nullptr;
        state->buffer_size = 0;
        (void)setvbuf(state->file_stream, nullptr, _IONBF, 0);
    } else {
        // allocate internal buffer
        state->buffer_size = (config.ibuffer_size == 0) ? CR_LOG_SINK_FILE_BUFFER : (size_t)config.ibuffer_size;
        state->buffer      = (char *)malloc(state->buffer_size);
        if (!state->buffer) {
            perror("(malloc) file sink buffer allocation failed");
            return (cr_log_sink_t) { 0 };
        }
    }

    if (!config.formatter) {
        // use default formatter
        config.formatter = cr_log__sink_file_format;
    }

    state->offset = 0;
    state->config = config;

    return (cr_log_sink_t) {
        .state   = state,
        .process = cr_log__sink_file_process,
        .flush   = cr_log__sink_file_flush,
        .free    = cr_log__sink_file_free,
    };
}

int
cr_log__sink_file_format(
    char *buf, size_t size, const cr_log_item_t *item, const struct cr_log_sink_file_config_t *config)
{
    int color = (int)config->color;
    return snprintf(
        buf,
        size,
        "[%ld.%ld] [%s%s%s] [%s] [%s:%d %s] ",
        // time
        item->time.tv_sec,
        item->time.tv_nsec,
        // level
        color ? cr_log_colors[item->level] : "",
        cr_log_level_names[item->level],
        color ? cr_log_reset : "",
        // scope
        item->scope[0] == '\0' ? " " : item->scope,
        // location
        item->filename,
        item->line,
        item->function);
}

void
cr_log__sink_file_process(void *sink_state, const cr_log_item_t *item)
{
    cr_log_sink_file_state_t *state = sink_state;
    if (item->level < state->config.level) {
        return;
    }

    char buf[CR_LOG_SINK_FILE_FORMAT_MAX];
    state->config.formatter(buf, sizeof(buf), item, &state->config);

    if (state->buffer_size == 0) {
        // bypass buffer, write directly
        (void)fprintf(state->file_stream, "%s%s\n", buf, item->buffer);
        return;
    }

    // try
    int offset = snprintf(
        state->buffer + state->offset, state->buffer_size - (size_t)state->offset, "%s%s\n", buf, item->buffer);

    if (offset < 0) {
        perror("(snprintf) failed to append to file sink buffer");
        return;
    }

    if (offset > (int)state->buffer_size) {
        // larger than buffer, write through
        cr_log__sink_file_flush(sink_state);
        (void)fprintf(state->file_stream, "%s%s\n", buf, item->buffer);
        return;
    }

    if (state->offset + offset >= (int)state->buffer_size) {
        // buffer overflow, flush and retry
        cr_log__sink_file_flush(sink_state);
        int ret = snprintf(
            state->buffer + state->offset, state->buffer_size - (size_t)state->offset, "%s%s\n", buf, item->buffer);

        if (ret < 0) {
            perror("(snprintf) failed to append to file sink buffer");
            return;
        }
        state->offset += ret;
    }

    state->offset += offset;
}

void
cr_log__sink_file_flush(void *sink_state)
{
    cr_log_sink_file_state_t *state = sink_state;

    // write buffer to file and flush
    (void)fwrite(state->buffer, 1, (size_t)state->offset, state->file_stream);
    (void)fflush(state->file_stream);
    state->offset = 0;
}

void
cr_log__sink_file_free(void *sink_state)
{
    struct cr_log_sink_file_state_t *state = sink_state;

    cr_log__sink_file_flush(state);
    if (!state->config.disable_close) {
        (void)fclose(state->file_stream);
    }

    free(state->buffer);
    free(state);
}
// }}}
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
