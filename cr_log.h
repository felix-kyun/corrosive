/*
    cr_log.h - v0.7.3 - Logging Library

    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/corrosive

    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob

    For other informations, see the end of this file.
 */

#ifndef CR_LOG_H
#define CR_LOG_H

#define _POSIX_C_SOURCE 202405L
#include <stddef.h>
#include <stdint.h>

#define CR_LOG_LEVEL_TRACE 0
#define CR_LOG_LEVEL_DEBUG 1
#define CR_LOG_LEVEL_INFO  2
#define CR_LOG_LEVEL_WARN  3
#define CR_LOG_LEVEL_ERROR 4
#define CR_LOG_LEVEL_FATAL 5
#define CR_LOG_LEVEL_OFF   6

#ifndef CR_LOG_PURGE_LEVEL
#define CR_LOG_PURGE_LEVEL CR_LOG_LEVEL_TRACE
#endif

// safe upto 1 << 16 (limited by uint16_t)
// intern tables are per instance
#ifndef CR_LOG_ITABLE_SIZE
#define CR_LOG_ITABLE_SIZE (1 << 8)
#endif

#ifndef CR_LOG_QUEUE_SIZE
#define CR_LOG_QUEUE_SIZE (1 << 12)
#endif

#ifndef CR_LOG_QUEUE_ITEM_SIZE
#define CR_LOG_QUEUE_ITEM_SIZE 512
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS
#define CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS 128
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF
#define CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF 1024
#endif

// more instances will increase memory overhead per thread
// instance id is monotonic
// as such, max_instance determines the max logger instance over app lifetime
#ifndef CR_LOG_MAX_INSTANCES
#define CR_LOG_MAX_INSTANCES 16
#endif

// purge
#define CR_LOG(ctx, level, ...)   cr_log(ctx, level, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define CR_LOG_GLOBAL(level, ...) cr_log(global_ctx, level, __FILE__, __LINE__, __func__, __VA_ARGS__)

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_TRACE
#define cr_log_trace_ctx(ctx, fmt, ...) CR_LOG(ctx, CR_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define cr_log_trace(fmt, ...)          CR_LOG_GLOBAL(CR_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
#define cr_log_trace_ctx(...) ((void)0)
#define cr_log_trace(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_DEBUG
#define cr_log_debug_ctx(ctx, fmt, ...) CR_LOG(ctx, CR_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define cr_log_debug(fmt, ...)          CR_LOG_GLOBAL(CR_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define cr_log_debug_ctx(...) ((void)0)
#define cr_log_debug(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_INFO
#define cr_log_info_ctx(ctx, fmt, ...) CR_LOG(ctx, CR_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define cr_log_info(fmt, ...)          CR_LOG_GLOBAL(CR_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define cr_log_info_ctx(...) ((void)0)
#define cr_log_info(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_WARN
#define cr_log_warn_ctx(ctx, fmt, ...) CR_LOG(ctx, CR_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define cr_log_warn(fmt, ...)          CR_LOG_GLOBAL(CR_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#else
#define cr_log_warn_ctx(...) ((void)0)
#define cr_log_warn(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_ERROR
#define cr_log_error_ctx(ctx, fmt, ...) CR_LOG(ctx, CR_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define cr_log_error(fmt, ...)          CR_LOG_GLOBAL(CR_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
#define cr_log_error_ctx(...) ((void)0)
#define cr_log_error(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_FATAL
#define cr_log_fatal_ctx(ctx, fmt, ...) CR_LOG(ctx, CR_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#define cr_log_fatal(fmt, ...)          CR_LOG_GLOBAL(CR_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#else
#define cr_log_fatal_ctx(...) ((void)0)
#define cr_log_fatal(...)     ((void)0)
#endif

typedef uint8_t            cr_log_level;
typedef struct cr_log_ctx  cr_log_ctx;
typedef struct cr_log_sink cr_log_sink;

cr_log_ctx *cr_log_new_ctx();
void        cr_log_flush_ctx(cr_log_ctx *ctx);
void        cr_log_destory_ctx(cr_log_ctx *ctx);
void        cr_log_set_level_ctx(cr_log_ctx *ctx, cr_log_level level);
void        cr_log_scope_set_ctx(cr_log_ctx *ctx, const char *scope);
int         cr_log_sink_add_ctx(cr_log_ctx *ctx, cr_log_level level, cr_log_sink sink);

[[gnu::hot, gnu::format(__printf__, 6, 7)]]
void cr_log(cr_log_ctx *ctx, cr_log_level level, const char *file, int line, const char *func, const char *fmt, ...);

// global logger
cr_log_ctx *global_ctx;

#define cr_log_init()                global_ctx = cr_log_new_ctx()
#define cr_log_flush()               cr_log_flush_ctx(global_ctx)
#define cr_log_destroy()             cr_log_destory_ctx(global_ctx)
#define cr_log_set_level(level)      cr_log_set_level_ctx(global_ctx, level)
#define cr_log_scope_set(scope)      cr_log_scope_set_ctx(global_ctx, scope)
#define cr_log_sink_add(level, sink) cr_log_sink_add_ctx(global_ctx, level, sink)

// default sink
#define cr_log_sink_default() cr_log_sink_fd_new(.fd = STDERR_FILENO, .level = cr_log_levelRACE, .bsize = 0)

// ** FD sink
struct cr_log_sink_fd_config {
    int    fd;
    size_t bsize;
};
cr_log_sink cr_log__sink_fd_new(struct cr_log_sink_fd_config config);
#define cr_log_sink_fd(...) cr_log__sink_fd_new((struct cr_log_sink_fd_config) { __VA_ARGS__ })

// ** file sink
typedef struct cr_log_sink_file_config {
    const char *target;
    bool        truncate;
    size_t      bsize;
} cr_log_sink_file_configt;

cr_log_sink cr_log__sink_file_new(struct cr_log_sink_file_config config);
#define cr_log_sink_file(...) cr_log__sink_file_new((struct cr_log_sink_file_config) { __VA_ARGS__ })

#ifdef CR_LOG_TELEMETRY
#define cr_log_get_dropped() cr_log_get_dropped_ctx(global_ctx)
uint64_t cr_log_get_dropped_ctx(cr_log_ctx *ctx);
#endif

#if defined(CR_LOG_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

/* Implementation */

#if (CR_LOG_ITABLE_SIZE & (CR_LOG_ITABLE_SIZE - 1)) != 0
#error "CR_LOG_ITABLE_SIZE must be a power of 2"
#endif

#if CR_LOG_ITABLE_SIZE >= (1 << 16)
#error "CR_LOG_ITABLE_SIZE must be less than 2^16 (limited by uint16_t)"
#endif

#if (CR_LOG_QUEUE_SIZE & (CR_LOG_ITABLE_SIZE - 1)) != 0
#error "CR_LOG_QUEUE_SIZE must be a power of 2"
#endif

#include <fcntl.h>
#include <immintrin.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef uint8_t   u8;
typedef int32_t   i32;
typedef uint32_t  u32;
typedef int64_t   i64;
typedef uint64_t  u64;
typedef size_t    usize;
typedef ptrdiff_t isize;

#define CACHE_LINE_SIZE 64

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

#ifdef CR_LOG_TELEMETRY
alignas(CACHE_LINE_SIZE) atomic_uint_fast64_t drop_count;
#endif

// monotonic instance id counter
static atomic_uint_fast16_t instance_id_counter = 0;

// per thread scope id storage
// id is a index into the per-instance intern table
thread_local struct {
    uint16_t scope_id;
} per_instance_scope[CR_LOG_MAX_INSTANCES];

// clang-format off
static constexpr size_t queue_size = CR_LOG_QUEUE_SIZE;
static constexpr size_t buffer_size
    = CR_LOG_QUEUE_ITEM_SIZE - (0
    + CACHE_LINE_SIZE			// sequence
    + sizeof(u8) 			// level
    + sizeof(u32) 			// line
    + sizeof(const char *)		// filename
    + sizeof(const char *)		// function
    + sizeof(const char *)		// scope
    + sizeof(struct timespec)	// time
    + sizeof(i64)			// scope_id
    );
// clang-format on

typedef struct cr_log_item {
    u8              level;
    u32             line;
    const char     *filename;
    const char     *function;
    const char     *scope;
    struct timespec time;
    i64             scope_id;
    char            buffer[buffer_size];
} cr_log_item;

typedef struct cr_log_queue {
    alignas(CACHE_LINE_SIZE) atomic_size_t write;
    alignas(CACHE_LINE_SIZE) atomic_size_t read;

    usize mask;
    sem_t items;

    struct {
        alignas(CACHE_LINE_SIZE) atomic_size_t sequence;
        struct cr_log_item meta;
    } buffer[queue_size];
} cr_log_queue;

static_assert(
    offsetof(cr_log_queue, read) == CACHE_LINE_SIZE,
    "cr_log_queue-> read/write are not aligned and might share a cache line");

typedef struct cr_log_itable {
    pthread_mutex_t write_lock;
    struct {
        u64   hash;
        char *key;
        bool  used;
    } items[CR_LOG_ITABLE_SIZE];
    uint16_t mask;
} cr_log_itable;

typedef struct cr_log_ctx {
    // monotonic instance id
    // used internally for matching thread local contexts across multiple instance
    uint16_t instance_id;

    // runtime log level
    cr_log_level level;

    // are we running?
    atomic_bool state;

    // list of sinks
    cr_log_sink *sinks_head;
    cr_log_sink *sinks_tail;

    // worker threads
    pthread_t consumer;

#ifdef CR_LOG_TELEMETRY
    alignas(CACHE_LINE_SIZE) atomic_uint_fast64_t dropped;
#endif

    // intern table
    // contains scope names, thread names
    alignas(CACHE_LINE_SIZE) cr_log_itable itable;

    // mpsc queue
    alignas(CACHE_LINE_SIZE) cr_log_queue queue;
} cr_log_ctx;

static int   enqueue(cr_log_ctx *ctx, struct cr_log_item meta);
static void *dequeue(void *arg);
static void  queue_consumer(cr_log_ctx *ctx, struct cr_log_item *item);

// writer interface
typedef struct cr_log_writer {
    i32 fd;

    // buffer
    usize bsize;
    char *buffer;
    usize bpos;
} cr_log_writer;

static cr_log_writer *writer_create(int target_fd, usize bsize);
static i32            writer_flush(cr_log_writer *writer);
static void           writer_destroy(cr_log_writer *writer);

static i32 writer_write(cr_log_writer *writer, const void *src, usize size);
static i32 writer_str(cr_log_writer *writer, const char *str);
static i32 writer_i64(cr_log_writer *writer, i64 value);
static i32 writer_u64(cr_log_writer *writer, u64 value);

static inline char *writer_reserve(cr_log_writer *writer, usize size);
static inline void  writer_advance(cr_log_writer *writer, usize size);

typedef struct cr_log_sink {
    struct cr_log_sink *__next;

    cr_log_level level;
    void        *state;

    void (*process)(void *sink_state, const cr_log_item *item);
    void (*flush)(void *sink_state);
    void (*free)(void *sink_state);
} cr_log_sink;

//! sink using fd should inherit sink_fd_base_t
typedef struct sink_fd_base {
    cr_log_writer *writer;
} sink_fd_base;

// FD Sink
typedef sink_fd_base sink_fd_state;
void                 sink_fd_flush(void *_state);
void                 sink_fd_process(void *_state, const cr_log_item *item);
void                 sink_fd_free(void *_state);

// File Sink
typedef struct sink_file_state {
    sink_fd_base                   base;
    struct cr_log_sink_file_config config;
} sink_file_state;

// uses sink_fd methods for flush and process
void sink_file_free(void *sink_state);

/*
 * Definations
 */

cr_log_ctx *
cr_log_new_ctx()
{
    cr_log_ctx *ctx = malloc(sizeof(cr_log_ctx));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->instance_id = atomic_fetch_add_explicit(&instance_id_counter, 1, memory_order_relaxed);
    ctx->level       = CR_LOG_LEVEL_INFO;
    atomic_store_relaxed(&ctx->state, true);
    ctx->sinks_head = nullptr;
    ctx->sinks_tail = nullptr;

#ifdef CR_LOG_TELEMETRY
    atomic_init(&ctx->dropped, 0);
#endif

    // itable
    pthread_mutex_init(&ctx->itable.write_lock, NULL);
    ctx->itable.mask = (CR_LOG_ITABLE_SIZE)-1;
    for (int i = 0; i < CR_LOG_ITABLE_SIZE; i++) {
        ctx->itable.items[i].used = false;
    }

    // queue
    for (usize i = 0; i < queue_size; i++) {
        atomic_init(&ctx->queue.buffer[i].sequence, i);
    }
    ctx->queue.mask = queue_size - 1;
    atomic_init(&ctx->queue.read, 0);
    atomic_init(&ctx->queue.write, 0);
    sem_init(&ctx->queue.items, 0, 0);
    pthread_create(&ctx->consumer, NULL, dequeue, ctx);

    return ctx;
}

void
cr_log_set_level_ctx(cr_log_ctx *ctx, cr_log_level level)
{
    ctx->level = level;
}

void
cr_log_flush_ctx(cr_log_ctx *ctx)
{
    cr_log_sink *sink = ctx->sinks_head;
    while (sink != NULL) {
        sink->flush(sink->state);
        sink = sink->__next;
    }
}

int
cr_log_sink_add_ctx(cr_log_ctx *ctx, cr_log_level level, cr_log_sink sink)
{
    cr_log_sink *new_sink = malloc(sizeof(cr_log_sink));
    if (new_sink == NULL) {
        return -1;
    }
    *new_sink        = sink;
    new_sink->level  = level;
    new_sink->__next = NULL;

    // append sink
    if (ctx->sinks_tail == NULL) {
        ctx->sinks_head = new_sink;
    } else {
        ctx->sinks_tail->__next = new_sink;
    }
    ctx->sinks_tail = new_sink;

    return 0;
}

void
cr_log_destory_ctx(cr_log_ctx *ctx)
{
    atomic_store_relaxed(&ctx->state, false);
    sem_post(&ctx->queue.items);
    pthread_join(ctx->consumer, NULL);
    sem_destroy(&ctx->queue.items);

    pthread_mutex_lock(&ctx->itable.write_lock);
    for (i32 i = 0; i < (CR_LOG_ITABLE_SIZE); i++) {
        if (ctx->itable.items[i].used) {
            free(ctx->itable.items[i].key);
            ctx->itable.items[i].used = false;
        }
    }
    pthread_mutex_unlock(&ctx->itable.write_lock);
    pthread_mutex_destroy(&ctx->itable.write_lock);

    cr_log_sink *sink = ctx->sinks_head;
    while (sink != NULL) {
        sink->free(sink->state);
        cr_log_sink *next = sink->__next;
        free(sink);
        sink = next;
    }

    free(ctx);
}

int
try_enqueue(cr_log_ctx *ctx, struct cr_log_item *meta)
{
    for (;;) {
        usize write_pos = atomic_load_relaxed(&ctx->queue.write);
        usize idx       = write_pos & ctx->queue.mask;
        usize seq       = atomic_load_relaxed(&ctx->queue.buffer[idx].sequence);

        // check availablity
        if (write_pos - seq == 0) {
            // available, try cas
            if (atomic_cas(&ctx->queue.write, &write_pos, write_pos + 1)) {
                // claimed
                ctx->queue.buffer[idx].meta = *meta;
                atomic_store_release(&ctx->queue.buffer[idx].sequence, write_pos + 1);
                sem_post(&ctx->queue.items);
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
enqueue(cr_log_ctx *ctx, struct cr_log_item meta)
{
    i32 backoff = 1;

    for (int i = 0; i < CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS; i++) {
        i32 ret = try_enqueue(ctx, &meta);
        if (ret == 0) {
            return 0;
        }
        if (ret == -1) {
            // spin and retry
            // exponential backoff
            for (i32 j = 0; j < backoff; j++) {
                _mm_pause();
            }
            backoff = (backoff < CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF) ? backoff << 1 : CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF;
        }
    }

    // drop
#ifdef CR_LOG_TELEMETRY
    atomic_fetch_add(&ctx->dropped, 1);
#endif
    return -1;
}

int
try_dequeue(cr_log_ctx *ctx)
{
    usize read_pos = atomic_load_relaxed(&ctx->queue.read);
    usize idx      = read_pos & ctx->queue.mask;
    usize seq      = atomic_load_acquire(&ctx->queue.buffer[idx].sequence);
    isize diff     = (isize)(seq - (read_pos + 1));

    if (diff == 0) {
        // consume
        queue_consumer(ctx, &ctx->queue.buffer[idx].meta);

        // release
        atomic_store_relaxed(&ctx->queue.buffer[idx].sequence, read_pos + queue_size);
        atomic_store_relaxed(&ctx->queue.read, read_pos + 1);

        return 0;
    }

    return -1;
}

void *
dequeue([[maybe_unused]] void *arg)
{
    cr_log_ctx *ctx = (cr_log_ctx *)arg;
    for (;;) {
        sem_wait(&ctx->queue.items);
        while (try_dequeue(ctx) == 0) {
            if (sem_trywait(&ctx->queue.items) != 0) {
                break;
            }
        }

        // drain on shutdown
        if (!atomic_load_relaxed(&ctx->state)) {
            while (try_dequeue(ctx) == 0) { }
            break;
        }
    }

    return NULL;
}

#ifdef CR_LOG_TELEMETRY
uint64_t
cr_log_get_dropped_ctx(cr_log_ctx *ctx)
{
    return atomic_load(&ctx->dropped);
}
#endif

void
cr_log(cr_log_ctx *ctx, cr_log_level level, const char *file, i32 line, const char *func, const char *fmt, ...)
{
    // runtime purge
    if (level < ctx->level) {
        return;
    }

    // clang-format off
    cr_log_item event = {
        .level     = level,
        .time = { 0 },
        .filename  = file,
        .line      = (u32)line,
        .function  = func,
        .scope_id = per_instance_scope[ctx->instance_id].scope_id
    };
    // clang-format on

    clock_gettime(CLOCK_REALTIME_COARSE, &event.time);

    va_list args;
    va_start(args, fmt);
    (void)vsnprintf(event.buffer, buffer_size, fmt, args);
    va_end(args);

    enqueue(ctx, event);
}

// * Scope
#define FNV1A_64_PRIME  0x00000100000001b3ULL
#define FNV1A_64_OFFSET 0xcbf29ce484222325ULL

static inline uint64_t
hash_string(const char *_key)
{
    u8 *key  = (u8 *)_key;
    u64 hash = FNV1A_64_OFFSET;
    while (*key) {
        hash ^= *key++;
        hash *= FNV1A_64_PRIME;
    }

    return hash;
}

void
cr_log_scope_set_ctx(cr_log_ctx *ctx, const char *scope)
{
    auto     hash = hash_string(scope);
    uint16_t idx  = hash & ctx->itable.mask;

    pthread_mutex_lock(&ctx->itable.write_lock);
    for (i32 i = 0; i < (CR_LOG_ITABLE_SIZE); i++) {
        auto entry = &ctx->itable.items[idx];

        // 0 is reserved for "default" scope
        if (idx != 0 && !entry->used) {
            entry->key = strdup(scope);
            if (!entry->key) {
                per_instance_scope[ctx->instance_id].scope_id = 0;
                goto cleanup;
            }

            entry->used                                   = true;
            entry->hash                                   = hash;
            per_instance_scope[ctx->instance_id].scope_id = idx;
            goto cleanup;
        } else if (entry->hash == hash && strcmp(entry->key, scope) == 0) {
            per_instance_scope[ctx->instance_id].scope_id = idx;
            goto cleanup;
        } else {
            idx = (idx + 1) & ctx->itable.mask;
        }
    }
    per_instance_scope[ctx->instance_id].scope_id = 0;

cleanup:
    pthread_mutex_unlock(&ctx->itable.write_lock);
}

static inline const char *
scope_get(cr_log_ctx *ctx, int64_t sid)
{
    if (sid == -1) {
        return "";
    }

    return ctx->itable.items[sid].key;
}

void
queue_consumer(cr_log_ctx *ctx, struct cr_log_item *item)
{
    item->scope = scope_get(ctx, item->scope_id);

    cr_log_sink *sink = ctx->sinks_head;
    while (sink != NULL) {
        if (sink->level <= item->level) {
            sink->process(sink->state, item);
        }
        sink = sink->__next;
    }
}

// * Writer
static cr_log_writer *
writer_create(i32 target_fd, usize bsize)
{
    if (target_fd < 0) {
        return NULL;
    }

    cr_log_writer *writer = malloc(sizeof(cr_log_writer));
    if (writer == NULL) {
        return NULL;
    }

    writer->fd    = target_fd;
    writer->bpos  = 0;
    writer->bsize = bsize;

    // buffered
    if (bsize > 0) {
        writer->buffer = malloc(bsize);
        if (writer->buffer == NULL) {
            free(writer);
            return NULL;
        }
    } else {
        writer->buffer = NULL;
    }

    return writer;
}

static inline char *
writer_reserve(cr_log_writer *writer, usize size)
{
    // passthrough for unbuffered writes
    if (writer->bsize == 0) {
        return NULL;
    }

    if (likely(writer->bpos + size <= writer->bsize)) {
        return writer->buffer + writer->bpos;
    }

    if (size <= writer->bsize) {
        writer_flush(writer);
        return writer->buffer + writer->bpos;
    }

    // failed to reserve
    // flush to prepare for write through
    writer_flush(writer);
    return NULL;
}

[[__gnu__::__always_inline__]]
static inline void
writer_advance(cr_log_writer *writer, usize size)
{
    writer->bpos += size;
}

i32
writer_write(cr_log_writer *writer, const void *src, usize size)
{
    char *dst = writer_reserve(writer, size);
    if (likely(dst)) {
        memcpy(dst, src, size);
        writer_advance(writer, size);
    } else {
        // bypass buffer
        usize written = 0;
        while (written < size) {
            isize current_write = write(writer->fd, src + written, size - written);
            if (current_write < 0) {
                perror("write");
                return -1;
            }
            written += (usize)current_write;
        }
    }

    return 0;
}

i32
writer_str(cr_log_writer *writer, const char *str)
{
    return writer_write(writer, str, strlen(str));
}

inline i32
writer_flush(cr_log_writer *writer)
{
    usize idx = 0;
    while (idx < writer->bpos) {
        isize written = write(writer->fd, writer->buffer + idx, writer->bpos - idx);
        if (written < 0) {
            perror("write");
            writer->bpos = 0;
            return -1;
        }
        idx += (usize)written;
    }
    writer->bpos = 0;
    return 0;
}

void
writer_destroy(cr_log_writer *writer)
{
    if (writer) {
        writer_flush(writer);
        if (writer->buffer) {
            free(writer->buffer);
        }
        free(writer);
    }
}

// NOLINTBEGIN(readability-magic-numbers)

static const u64 pow10_table[] = {
    1ULL,                   // 10^0
    10ULL,                  // 10^1
    100ULL,                 // 10^2
    1000ULL,                // 10^3
    10000ULL,               // 10^4
    100000ULL,              // 10^5
    1000000ULL,             // 10^6
    10000000ULL,            // 10^7
    100000000ULL,           // 10^8
    1000000000ULL,          // 10^9
    10000000000ULL,         // 10^10
    100000000000ULL,        // 10^11
    1000000000000ULL,       // 10^12
    10000000000000ULL,      // 10^13
    100000000000000ULL,     // 10^14
    1000000000000000ULL,    // 10^15
    10000000000000000ULL,   // 10^16
    100000000000000000ULL,  // 10^17
    1000000000000000000ULL, // 10^18
    10000000000000000000ULL // 10^19
};

//! x => floor(log10(x))
static inline u8
fast_log10(u64 value)
{
    //! x  =>  floor(floor(log2(x)) * log10(2))
    u8 guess = ((63 - (u8)__builtin_clzll(value | 1ULL)) * 1233) >> 12;
    u8 next  = guess + (guess < 19);
    return guess + (value >= pow10_table[next]);
}

static inline u64
fast_abs(i64 value)
{
    u64 mask = (u64)value >> 63;
    return ((u64)value + mask) ^ mask;
}

static const char digit_pairs[] = "00010203040506070809"
                                  "10111213141516171819"
                                  "20212223242526272829"
                                  "30313233343536373839"
                                  "40414243444546474849"
                                  "50515253545556575859"
                                  "60616263646566676869"
                                  "70717273747576777879"
                                  "80818283848586878889"
                                  "90919293949596979899";
i32
writer_i64(cr_log_writer *writer, i64 value)
{
    char stack_buffer[32];
    u64  uvalue = fast_abs(value);
    // adjust to account for fast_log10 flooring and sign bit
    u8    len     = fast_log10(uvalue) + 1 + (value < 0);
    char *ibuffer = writer_reserve(writer, len);

    if (!ibuffer) {
        ibuffer = stack_buffer;
    }

    char *idx = ibuffer + len;

    if (uvalue == 0) {
        *--idx = '0';
        goto commit;
    }

    while (uvalue >= 10) {
        u64 rem = uvalue % 100;
        uvalue /= 100;
        rem *= 2;

        idx -= 2;
        idx[0] = digit_pairs[rem];
        idx[1] = digit_pairs[rem + 1];
    }

    if (uvalue > 0) {
        *--idx = (char)('0' + uvalue);
    }

    if (value < 0) {
        *--idx = '-';
    }

commit:
    if (ibuffer == stack_buffer) {
        return writer_write(writer, idx, (usize)(ibuffer + len - idx));
    }

    writer_advance(writer, len);
    return 0;
}

i32
writer_u64(cr_log_writer *writer, u64 value)
{
    char stack_buffer[32];
    // adjust to account for fast_log10 flooring
    u8    len     = fast_log10(value) + 1;
    char *ibuffer = writer_reserve(writer, len);

    if (!ibuffer) {
        ibuffer = stack_buffer;
    }

    char *idx = ibuffer + len;

    if (value == 0) {
        *--idx = '0';
        goto commit;
    }

    while (value >= 10) {
        u64 rem = value % 100;
        value /= 100;
        rem *= 2;

        idx -= 2;
        idx[0] = digit_pairs[rem];
        idx[1] = digit_pairs[rem + 1];
    }

    if (value > 0) {
        *--idx = (char)('0' + value);
    }

commit:
    if (ibuffer == stack_buffer) {
        return writer_write(writer, idx, (usize)(ibuffer + len - idx));
    }

    writer_advance(writer, len);
    return 0;
}

// NOLINTEND(readability-magic-numbers)

// * Sinks
cr_log_sink
cr_log__sink_fd_new(struct cr_log_sink_fd_config config)
{
    sink_fd_state *state = malloc(sizeof(sink_fd_state));
    if (!state) {
        perror("(malloc) fd sink allocation failed");
        goto finish;
    }

    state->writer = writer_create(config.fd, config.bsize);
    if (!state->writer) {
        perror("(writer_create) writer creation failed");
        goto finish;
    }

finish:
    return (cr_log_sink) { //
                           .state   = state,
                           .process = sink_fd_process,
                           .flush   = sink_fd_flush,
                           .free    = sink_fd_free
    };
}

void
sink_fd_flush(void *_state)
{
    auto state = (sink_fd_base *)_state;
    writer_flush(state->writer);
}

void
sink_fd_process(void *_state, const cr_log_item *item)
{
    auto state  = (sink_fd_base *)_state;
    auto writer = state->writer;

    // NOLINTBEGIN(readability-magic-numbers)
    writer_write(writer, "[", 1);
    writer_i64(writer, item->time.tv_sec);
    writer_write(writer, ".", 1);
    writer_i64(writer, item->time.tv_nsec);
    writer_write(writer, "] [", 3);
    writer_write(writer, cr_log_colors[item->level], 6);
    writer_str(writer, cr_log_level_names[item->level]);
    writer_write(writer, cr_log_reset, 5);
    writer_write(writer, "] [", 3);
    writer_str(writer, item->scope);
    writer_write(writer, "] [", 3);
    writer_str(writer, item->filename);
    writer_write(writer, ":", 1);
    writer_u64(writer, item->line);
    writer_write(writer, " ", 1);
    writer_str(writer, item->function);
    writer_write(writer, "] ", 2);
    writer_str(writer, item->buffer);
    writer_write(writer, "\n", 1);
    // NOLINTEND(readability-magic-numbers)
}

void
sink_fd_free(void *_state)
{
    auto state = (sink_fd_base *)_state;
    sink_fd_flush(state);
    writer_destroy(state->writer);
    free(state);
}

cr_log_sink
cr_log__sink_file_new(struct cr_log_sink_file_config config)
{
    sink_file_state *state = malloc(sizeof(sink_file_state));
    if (!state) {
        perror("(malloc) file sink state allocation failed");
        return (cr_log_sink) { 0 };
    }

    int fd   = STDERR_FILENO;
    int mode = O_WRONLY | O_CREAT | O_APPEND;
    if (config.truncate) {
        mode |= O_TRUNC;
    }
    if (config.target != nullptr) {
        fd = open(config.target, mode, 0644);
    } else {
        err("fallback to stderr");
    }

    state->config = config;
    state->base   = (sink_fd_base) {
        .writer = writer_create(fd, config.bsize),
    };

    return (cr_log_sink) {
        .state   = state,
        .process = sink_fd_process,
        .flush   = sink_fd_flush,
        .free    = sink_file_free,
    };
}

void
sink_file_free(void *sink_state)
{
    struct sink_file_state *state = sink_state;
    int                     fd    = state->base.writer->fd;
    sink_fd_flush(state);
    writer_destroy(state->base.writer);
    close(fd);
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
