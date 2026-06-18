/*
    cr_log.h - v0.7.3 - Logging Library

    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/corrosive

    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob

    For other informations, see the end of this file.
 */

/*
 * Table Of Contents
 * zones
 * 	  public
 *		configs
 *		macros
 *		types
 *		declarations
 * 	  private
 *		configs
 *		macros
 *		types
 *		declarations
 *		statics
 *		helpers
 *		definitions
 */

#ifndef CR_LOG_H
#define CR_LOG_H

/***********************
 * zone:public:configs *
 ***********************/

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

/**********************
 * zone:public:macros *
 **********************/

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

/*********************
 * zone:public:types *
 *********************/

typedef uint8_t                 cr_log_level;
typedef struct cr_log_ctx       cr_log_ctx;
typedef struct cr_log_sink      cr_log_sink;
typedef struct cr_log_transport cr_log_transport;

/****************************
 * zone:public:declarations *
 ****************************/

cr_log_ctx *cr_log_new_ctx();
void        cr_log_flush_ctx(cr_log_ctx *ctx);
void        cr_log_destory_ctx(cr_log_ctx *ctx);
void        cr_log_set_level_ctx(cr_log_ctx *ctx, cr_log_level level);
void        cr_log_scope_set_ctx(cr_log_ctx *ctx, const char *scope);
int         cr_log_sink_add_ctx(cr_log_ctx *ctx, cr_log_level level, cr_log_sink *sink);

[[gnu::hot, gnu::format(__printf__, 6, 7)]]
void cr_log(cr_log_ctx *ctx, cr_log_level level, const char *file, int line, const char *func, const char *fmt, ...);

// global logger
extern cr_log_ctx *global_ctx;

#define cr_log_init()                global_ctx = cr_log_new_ctx()
#define cr_log_flush()               cr_log_flush_ctx(global_ctx)
#define cr_log_destroy()             cr_log_destory_ctx(global_ctx)
#define cr_log_set_level(level)      cr_log_set_level_ctx(global_ctx, level)
#define cr_log_scope_set(scope)      cr_log_scope_set_ctx(global_ctx, scope)
#define cr_log_sink_add(level, sink) cr_log_sink_add_ctx(global_ctx, level, sink)

// sink
#define cr_log_sink_text(...) cr_log_sink_text_new((cr_log_transport *[]) { __VA_ARGS__, nullptr })
cr_log_sink *cr_log_sink_text_new(cr_log_transport **transports);

// transport
cr_log_transport *cr_log_transport_fd(int fd);
cr_log_transport *cr_log_transport_file(const char *file, int flags);

#ifdef CR_LOG_TELEMETRY
#define cr_log_get_dropped() cr_log_get_dropped_ctx(global_ctx)
uint64_t cr_log_get_dropped_ctx(cr_log_ctx *ctx);
#endif

#if defined(CR_LOG_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

/*
 * ======================================================
 * ================== Implementation ====================
 * ======================================================
 */

/************************
 * zone:private:configs *
 ************************/

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

#ifndef CR_LOG_MAX_INSTANCES
#define CR_LOG_MAX_INSTANCES 16
#endif

#ifndef CR_LOG_ITEM_SIZE
#define CR_LOG_ITEM_SIZE (1 << 9)
#endif

#ifndef CR_LOG_ITEM_FIELDS
#define CR_LOG_ITEM_FIELDS 16
#endif

#ifndef CR_LOG_QUEUE_SIZE
#define CR_LOG_QUEUE_SIZE (1 << 12)
#endif

#ifndef CR_LOG_ITABLE_SIZE
#define CR_LOG_ITABLE_SIZE (1 << 8)
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS
#define CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS 128
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF
#define CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF 1024
#endif

#if (CR_LOG_ITEM_SIZE & (CR_LOG_ITEM_SIZE - 1)) != 0
#error "CR_LOG_ITEM_SIZE is not a power of 2"
#endif

#if (CR_LOG_QUEUE_SIZE & (CR_LOG_ITABLE_SIZE - 1)) != 0
#error "CR_LOG_QUEUE_SIZE must be a power of 2"
#endif

#if CR_LOG_ITABLE_SIZE >= (1 << 16)
#error "CR_LOG_ITABLE_SIZE must be less than 2^16 (limited by uint16_t)"
#endif

#if (CR_LOG_ITABLE_SIZE & (CR_LOG_ITABLE_SIZE - 1)) != 0
#error "CR_LOG_ITABLE_SIZE must be a power of 2"
#endif

#define CACHE_LINE_SIZE 64

/***********************
 * zone:private:macros *
 ***********************/

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

/**********************
 * zone:private:types *
 **********************/

static constexpr size_t queue_size  = CR_LOG_QUEUE_SIZE;
static constexpr size_t buffer_size = CR_LOG_ITEM_SIZE
    - (CACHE_LINE_SIZE           // sequence
       + sizeof(u8)              // level
       + sizeof(u32)             // line
       + sizeof(const char *)    // filename
       + sizeof(const char *)    // function
       + sizeof(const char *)    // scope
       + sizeof(struct timespec) // time
       + sizeof(i64)             // scope_id
    );

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

typedef void (*process_fn)(cr_log_sink *sink, cr_log_item *item);
struct cr_log_sink {
    struct cr_log_sink *next;
    process_fn          process;
    cr_log_level        level;

    struct cr_log_transport *transports;

    // buffer
    usize bpos;
    usize bsize;
    char *buffer;
};

struct cr_log_transport_ops {
    int (*write)(cr_log_transport *transport, void *src, size_t len);
    int (*close)(cr_log_transport *transport);
};

struct cr_log_transport {
    struct cr_log_transport     *next;
    struct cr_log_transport_ops *ops;
};

struct cr_log_transport_fd {
    struct cr_log_transport base;
    int                     fd;
};

/*****************************
 * zone:private:declarations *
 *****************************/

static cr_log_sink *cr_log__sink_new(process_fn process, usize bsize, struct cr_log_transport **transports);
static void         cr_log__sink_free(cr_log_sink *sink);
static i32          cr_log__sink_flush(cr_log_sink *sink);

static i32 cr_log__sink_write(cr_log_sink *sink, const void *src, usize size);
static i32 cr_log__sink_str(cr_log_sink *sink, const char *str);
static i32 cr_log__sink_u64(cr_log_sink *sink, u64 value);
static i32 cr_log__sink_i64(cr_log_sink *sink, i64 value);

static inline char *cr_log__sink_reserve(cr_log_sink *sink, usize size);
static inline void  cr_log__sink_advance(cr_log_sink *sink, usize size);

static void cr_log__sink_text_process(cr_log_sink *sink, cr_log_item *item);

static int cr_log__transport_fd_write(cr_log_transport *transport, void *src, usize size);
static int cr_log__transport_fd_close(cr_log_transport *transport);

static int cr_log__transport_file_close(cr_log_transport *transport);

static int   enqueue(cr_log_ctx *ctx, struct cr_log_item meta);
static void *dequeue(void *arg);
static void  queue_consumer(cr_log_ctx *ctx, struct cr_log_item *item);

/************************
 * zone:private:statics *
 ************************/

cr_log_ctx *global_ctx;

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

static struct cr_log_transport_ops fd_ops = {
    .write = cr_log__transport_fd_write,
    .close = cr_log__transport_fd_close,
};

static struct cr_log_transport_ops file_ops = {
    .write = cr_log__transport_fd_write,
    .close = cr_log__transport_file_close,
};

/************************
 * zone:private:helpers *
 ************************/

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
// NOLINTEND(readability-magic-numbers)

/****************************
 * zone:private:definitions *
 ****************************/

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
        cr_log__sink_flush(sink);
        sink = sink->next;
    }
}

int
cr_log_sink_add_ctx(cr_log_ctx *ctx, cr_log_level level, cr_log_sink *sink)
{
    if (sink == NULL) {
        return -1;
    }

    sink->level = level;

    // append sink
    if (ctx->sinks_tail == NULL) {
        ctx->sinks_head = sink;
    } else {
        ctx->sinks_tail->next = sink;
    }
    ctx->sinks_tail = sink;

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
        cr_log_sink *next = sink->next;
        cr_log__sink_free(sink);
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
            sink->process(sink, item);
        }
        sink = sink->next;
    }
}

static cr_log_sink *
cr_log__sink_new(process_fn process, usize bsize, struct cr_log_transport *transports[])
{
    cr_log_sink *sink = calloc(1, sizeof(cr_log_sink));
    if (sink == NULL) {
        return NULL;
    }

    if (bsize > 0) {
        sink->buffer = malloc(bsize);
        if (sink->buffer == NULL) {
            free(sink);
            return NULL;
        }
    }

    for (int i = 0; transports[i] != nullptr; i++) {
        transports[i]->next = transports[i + 1];
    }

    sink->bsize      = bsize;
    sink->transports = transports[0];
    sink->process    = process;

    return sink;
}

static void
cr_log__sink_free(cr_log_sink *sink)
{
    cr_log__sink_flush(sink);

    // close transports
    struct cr_log_transport *transport = sink->transports;
    while (transport != nullptr) {
        struct cr_log_transport *t = transport;
        t->ops->close(t);
        transport = t->next;
        free(t);
    }

    free(sink->buffer);
    free(sink);
}

static i32
cr_log__sink_flush(cr_log_sink *sink)
{
    struct cr_log_transport *transport = sink->transports;
    while (transport != nullptr) {
        transport->ops->write(transport, sink->buffer, sink->bpos);
        transport = transport->next;
    }

    sink->bpos = 0;
    return 0;
}

static inline char *
cr_log__sink_reserve(cr_log_sink *sink, usize size)
{
    if (sink->bsize == 0) {
        return nullptr;
    }

    if (likely(sink->bpos + size <= sink->bsize)) {
        return sink->buffer + sink->bpos;
    }

    if (size <= sink->bsize) {
        cr_log__sink_flush(sink);
        return sink->buffer;
    }

    // flush buffered writes
    // for writethrough
    cr_log__sink_flush(sink);
    return nullptr;
}

static inline void
cr_log__sink_advance(cr_log_sink *sink, usize size)
{
    sink->bpos += size;
}

static i32
cr_log__sink_write(cr_log_sink *sink, const void *src, usize size)
{
    char *dst = cr_log__sink_reserve(sink, size);
    if (likely(dst)) {
        memcpy(dst, src, size);
        cr_log__sink_advance(sink, size);
    } else {
        // writethrough
        struct cr_log_transport *transport = sink->transports;
        while (transport != nullptr) {
            transport->ops->write(transport, (void *)src, size);
            transport = transport->next;
        }
    }
    return 0;
}

static i32
cr_log__sink_str(cr_log_sink *sink, const char *str)
{
    return cr_log__sink_write(sink, str, strlen(str));
}

// NOLINTBEGIN(readability-magic-numbers)
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
static i32
cr_log__sink_u64(cr_log_sink *sink, u64 value)
{
    char stack_buffer[32];
    // adjust to account for fast_log10 flooring
    u8    len     = fast_log10(value) + 1;
    char *ibuffer = cr_log__sink_reserve(sink, len);

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
        return cr_log__sink_write(sink, idx, (usize)(ibuffer + len - idx));
    }

    cr_log__sink_advance(sink, len);
    return 0;
}

static i32
cr_log__sink_i64(cr_log_sink *sink, i64 value)
{
    char stack_buffer[32];
    u64  uvalue = fast_abs(value);
    // adjust to account for fast_log10 flooring and sign bit
    u8    len     = fast_log10(uvalue) + 1 + (value < 0);
    char *ibuffer = cr_log__sink_reserve(sink, len);

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
        return cr_log__sink_write(sink, idx, (usize)(ibuffer + len - idx));
    }

    cr_log__sink_advance(sink, len);
    return 0;
}
// NOLINTEND(readability-magic-numbers)

static void
cr_log__sink_text_process(cr_log_sink *sink, cr_log_item *item)
{

    // NOLINTBEGIN(readability-magic-numbers)
    cr_log__sink_write(sink, "[", 1);
    cr_log__sink_i64(sink, item->time.tv_sec);
    cr_log__sink_write(sink, ".", 1);
    cr_log__sink_i64(sink, item->time.tv_nsec);
    cr_log__sink_write(sink, "] [", 3);
    cr_log__sink_write(sink, cr_log_colors[item->level], 6);
    cr_log__sink_str(sink, cr_log_level_names[item->level]);
    cr_log__sink_write(sink, cr_log_reset, 5);
    cr_log__sink_write(sink, "] [", 3);
    cr_log__sink_str(sink, item->scope);
    cr_log__sink_write(sink, "] [", 3);
    cr_log__sink_str(sink, item->filename);
    cr_log__sink_write(sink, ":", 1);
    cr_log__sink_u64(sink, item->line);
    cr_log__sink_write(sink, " ", 1);
    cr_log__sink_str(sink, item->function);
    cr_log__sink_write(sink, "] ", 2);
    cr_log__sink_str(sink, item->buffer);
    cr_log__sink_write(sink, "\n", 1);
    // NOLINTEND(readability-magic-numbers)
}

cr_log_sink *
cr_log_sink_text_new(cr_log_transport **transports)
{
    return cr_log__sink_new(cr_log__sink_text_process, 8 * 1024, transports);
}

cr_log_transport *
cr_log_transport_fd(int fd)
{
    struct cr_log_transport_fd *transport = malloc(sizeof(struct cr_log_transport_fd));
    if (transport == NULL) {
        return NULL;
    }
    transport->base.ops = &fd_ops;
    transport->fd       = fd;

    return (cr_log_transport *)transport;
}

static int
cr_log__transport_fd_write(cr_log_transport *transport, void *src, size_t size)
{
    int fd = ((struct cr_log_transport_fd *)transport)->fd;

    while (size > 0) {
        isize written = write(fd, src, size);
        if (written < 0) {
            return -1;
        }
        size -= (usize)written;
        src += written;
    }

    return 0;
}

static int
cr_log__transport_fd_close(cr_log_transport *transport)
{
    (void)transport;
    return 0;
}

cr_log_transport *
cr_log_transport_file(const char *file, int flags)
{
    int               fd        = open(file, flags, 0644);
    cr_log_transport *transport = cr_log_transport_fd(fd);
    transport->ops              = &file_ops;
    return transport;
}

static int
cr_log__transport_file_close(cr_log_transport *transport)
{
    close(((struct cr_log_transport_fd *)transport)->fd);
    return 0;
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
        CR_LOG_PURGE_LEVEL
                Set this to appropriate CR_LOG_LEVEL_* to purge log function calls.
                Any log of a lower level is replaced by a (void(0)).
                Therefore, no runtime overhead.
        CR_LOG_MAX_INSTANCES
                This controls the upper limit on how many context can be created during app lifetime.
                This is necessary as this sets the size of how many scope_id to value any thread can handle.
                A value of 16 means, there can be total 16 context (including global one).
                Internally each context gets a monotonic context id,
                Which is used to look up what scope is set for any context per thread.
                Higher value will increase memory overhead per thread.
        CR_LOG_ITEM_SIZE
                Controls the size of individual items in queue.
        CR_LOG_ITEM_FIELDS
                Max capacity of each log item to hold fields.
                Fields can be either format parameter or key-value pair.
        CR_LOG_QUEUE_SIZE
                Size of the internal queue used to asyncronously dispatch log calls.
                Only tune this if you are working under extreme multithreadedthread load.
                The default value is usually more than enough.
                Profile using CR_LOG_TELEMETRY to see dropped items.
        CR_LOG_ITABLE_SIZE
                Used to set the size of intern table inside all the contexts.
                Only nessecary if you set scopes frequently (default size is 256).
                NOTE: max safe value is 1<<16 (limited by the capacity of uint16_t)
        CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS
                Max attempts to enqueue an item before dropping it.
        CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF
                Used to clamp backoff spin timing.

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
