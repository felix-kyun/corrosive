/*
 * cr_log.h - v0.9.0 - Logging Library
 *
 * Author:   Praise Jacob <iampraisejacob@gmail.com>
 * Repo:     https://github.com/felix-kyun/corrosive
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Praise Jacob
 *
 * A single header library that provides logging functions.
 * Part of the Corrosive library.
 *
 * To use this library, do this in *one* of your source files:
 * 		#define CR_LOG_IMPL
 * 		#include "cr_log.h"
 *
 * Table Of Contents
 * 	- Zones (code)
 *    	- public
 *    		- configs
 *    		- macros
 *    		- types
 *    		- declarations
 *     	- private
 *    		- configs
 *    		- macros
 *    		- types
 *    		- declarations
 *    		- statics
 *    		- helpers
 *    		- definitions
 *  - Compile time options
 *  - Documentation
 *  - Examples
 *  - License
 *  - Credits
 *
 * For other informations, see the end of this file.
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

/* Macros for auto mapping raw values to field types */

/*
 * CR_PP_EVAL(...)
 * force preprocessor rescans
 * at EVAL_4 with each macro containing 4 calls to previous level, this allows upto 4^4 (256) rescans
 */
#define CR_PP_EVAL_0(...) __VA_ARGS__
#define CR_PP_EVAL_1(...) CR_PP_EVAL_0(CR_PP_EVAL_0(CR_PP_EVAL_0(CR_PP_EVAL_0(__VA_ARGS__))))
#define CR_PP_EVAL_2(...) CR_PP_EVAL_1(CR_PP_EVAL_1(CR_PP_EVAL_1(CR_PP_EVAL_1(__VA_ARGS__))))
#define CR_PP_EVAL_3(...) CR_PP_EVAL_2(CR_PP_EVAL_2(CR_PP_EVAL_2(CR_PP_EVAL_2(__VA_ARGS__))))
#define CR_PP_EVAL_4(...) CR_PP_EVAL_3(CR_PP_EVAL_3(CR_PP_EVAL_3(CR_PP_EVAL_3(__VA_ARGS__))))
#define CR_PP_EVAL(...)   CR_PP_EVAL_4(__VA_ARGS__)

/*
 * CR_PP_DEFER(macro)
 * defer macro expansion, so that its not expanded immediately and cause potential a blue paint
 */
#define CR_PP_EMPTY()
#define CR_PP_DEFER(macro) macro CR_PP_EMPTY()

/*
 * CR_PP_BREAK(...)
 * empty function-like macro selected by CR_PP_CONSUME to terminate recursion
 */
#define CR_PP_BREAK(...)

/*
 * CR_PP_CONSUME
 * tries to consume sentinel and replace it with '0, CR_PP_BREAK'
 * to shift the arguments and make CR_PP_BREAK the selected function and terminate recursion
 */
#define CR_PP_CONSUME_0() 0, CR_PP_BREAK
#define CR_PP_CONSUME_1() CR_PP_CONSUME_0
#define CR_PP_CONSUME()   CR_PP_CONSUME_1

/*
 * CR_PP_SELECT(next, fn)
 * given next token and current function, select fn or break out using CR_PP_BREAK(...)
 *
 * Note:
 * 2 levels of indirection is needed as preprocessor splits arguments to a macro by comma before expanding them
 * as such, if only one level is used, 'CR_PP_CONSUME next' will always remain single argument
 * which breaks when CR_PP_CONSUME is replaced by '0, CR_PP_BREAK' on encountering sentinel value.
 */
#define CR_PP_SELECT_0(next, fn, ...) CR_PP_DEFER(fn)
#define CR_PP_SELECT_1(next, fn, ...) CR_PP_SELECT_0(next, fn)
#define CR_PP_SELECT(next, fn)        CR_PP_SELECT_1(CR_PP_CONSUME next, fn)

/*
 * CR_PP_MAP(fn, ...)
 * map all the values of __VA_ARGS__ over fn
 *
 * NOTE:
 * ()()() acts as sentinel
 * fn can either be macro/expr or actual fn
 * _0 and _1 are used to ping pong and avoid being painted blue by pre-processor
 */
#define CR_PP_MAP_0(fn, current, next, ...) fn(current) CR_PP_SELECT(next, CR_PP_MAP_1)(fn, next, __VA_ARGS__)
#define CR_PP_MAP_1(fn, current, next, ...) fn(current) CR_PP_SELECT(next, CR_PP_MAP_0)(fn, next, __VA_ARGS__)
#define CR_PP_MAP(fn, ...)                  CR_PP_EVAL(CR_PP_MAP_1(fn, __VA_ARGS__, ()()()))

/* convert supported types to cr_log_field */
#define CR_LOG_VALUE(v)                                                                                                \
    _Generic(                                                                                                          \
        (0, (v)),                                                                                                      \
        int8_t: cr_log_i64,                                                                                            \
        int16_t: cr_log_i64,                                                                                           \
        int32_t: cr_log_i64,                                                                                           \
        int64_t: cr_log_i64,                                                                                           \
        uint8_t: cr_log_u64,                                                                                           \
        uint16_t: cr_log_u64,                                                                                          \
        uint32_t: cr_log_u64,                                                                                          \
        uint64_t: cr_log_u64,                                                                                          \
        bool: cr_log_bool,                                                                                             \
        char *: cr_log_str,                                                                                            \
        struct cr_log_field: cr_log_pass)(v)

/* convert a key(string, not copied) value(copied) pair to cr_log_field */
#define CR_LOG_KV(k, v) cr_log_kv((k), CR_LOG_VALUE(v))

/* used to log variable values, auto convert var name to key */
#define CR_LOG_VAR(var) CR_LOG_KV(#var, var)

#define CR_LOG_AUTOWRAP(v) CR_LOG_VALUE(v),
#define cr_log(ctx, level, message, ...)                                                                               \
    cr_log_submit(                                                                                                     \
        ctx,                                                                                                           \
        level,                                                                                                         \
        __FILE__,                                                                                                      \
        __LINE__,                                                                                                      \
        __func__,                                                                                                      \
        message,                                                                                                       \
        (cr_log_field[]) {                                                                                             \
            __VA_OPT__(CR_PP_MAP(CR_LOG_AUTOWRAP, __VA_ARGS__))(cr_log_field) { .type = CR_LOG_TYPE_NONE } })

/* compile time pruging */
#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_TRACE
#define cr_log_trace_ctx(ctx, message, ...) cr_log(ctx, CR_LOG_LEVEL_TRACE, message __VA_OPT__(, ) __VA_ARGS__)
#define cr_log_trace(message, ...) cr_log(cr_log_global_ctx, CR_LOG_LEVEL_TRACE, message __VA_OPT__(, ) __VA_ARGS__)
#else
#define cr_log_trace_ctx(...) ((void)0)
#define cr_log_trace(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_DEBUG
#define cr_log_debug_ctx(ctx, message, ...) cr_log(ctx, CR_LOG_LEVEL_DEBUG, message __VA_OPT__(, ) __VA_ARGS__)
#define cr_log_debug(message, ...) cr_log(cr_log_global_ctx, CR_LOG_LEVEL_DEBUG, message __VA_OPT__(, ) __VA_ARGS__)
#else
#define cr_log_debug_ctx(...) ((void)0)
#define cr_log_debug(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_INFO
#define cr_log_info_ctx(ctx, message, ...) cr_log(ctx, CR_LOG_LEVEL_INFO, message __VA_OPT__(, ) __VA_ARGS__)
#define cr_log_info(message, ...) cr_log(cr_log_global_ctx, CR_LOG_LEVEL_INFO, message __VA_OPT__(, ) __VA_ARGS__)
#else
#define cr_log_info_ctx(...) ((void)0)
#define cr_log_info(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_WARN
#define cr_log_warn_ctx(ctx, message, ...) cr_log(ctx, CR_LOG_LEVEL_WARN, message __VA_OPT__(, ) __VA_ARGS__)
#define cr_log_warn(message, ...) cr_log(cr_log_global_ctx, CR_LOG_LEVEL_WARN, message __VA_OPT__(, ) __VA_ARGS__)
#else
#define cr_log_warn_ctx(...) ((void)0)
#define cr_log_warn(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_ERROR
#define cr_log_error_ctx(ctx, message, ...) cr_log(ctx, CR_LOG_LEVEL_ERROR, message __VA_OPT__(, ) __VA_ARGS__)
#define cr_log_error(message, ...) cr_log(cr_log_global_ctx, CR_LOG_LEVEL_ERROR, message __VA_OPT__(, ) __VA_ARGS__)
#else
#define cr_log_error_ctx(...) ((void)0)
#define cr_log_error(...)     ((void)0)
#endif

#if CR_LOG_PURGE_LEVEL <= CR_LOG_LEVEL_FATAL
#define cr_log_fatal_ctx(ctx, message, ...) cr_log(ctx, CR_LOG_LEVEL_FATAL, message __VA_OPT__(, ) __VA_ARGS__)
#define cr_log_fatal(message, ...) cr_log(cr_log_global_ctx, CR_LOG_LEVEL_FATAL, message __VA_OPT__(, ) __VA_ARGS__)
#else
#define cr_log_fatal_ctx(...) ((void)0)
#define cr_log_fatal(...)     ((void)0)
#endif

/*********************
 * zone:public:types *
 *********************/

typedef uint8_t                 cr_log_level;
typedef struct cr_log_ctx       cr_log_ctx;
typedef struct cr_log_field     cr_log_field;
typedef struct cr_log_item      cr_log_item;
typedef struct cr_log_sink      cr_log_sink;
typedef struct cr_log_transport cr_log_transport;

/****************************
 * zone:public:declarations *
 ****************************/

cr_log_ctx *cr_log_new_ctx();
void        cr_log_flush_ctx(cr_log_ctx *ctx);
void        cr_log_destory_ctx(cr_log_ctx *ctx);

void cr_log_set_level_ctx(cr_log_ctx *ctx, cr_log_level level);
void cr_log_scope_set_ctx(cr_log_ctx *ctx, const char *scope);
int  cr_log_sink_add_ctx(cr_log_ctx *ctx, cr_log_level level, cr_log_sink *sink);

cr_log_field        cr_log_i64(int64_t value);
cr_log_field        cr_log_u64(uint64_t value);
cr_log_field        cr_log_bool(bool value);
cr_log_field        cr_log_str(const char *value);
inline cr_log_field cr_log_pass(cr_log_field value);
inline cr_log_field cr_log_kv(const char *key, cr_log_field value);

[[gnu::hot]]
void cr_log_submit(
    cr_log_ctx   *ctx,
    cr_log_level  level,
    const char   *file,
    int           line,
    const char   *func,
    const char   *message,
    cr_log_field *fields);

// global logger
extern cr_log_ctx *cr_log_global_ctx;

#define cr_log_init()                cr_log_global_ctx = cr_log_new_ctx()
#define cr_log_flush()               cr_log_flush_ctx(cr_log_global_ctx)
#define cr_log_destroy()             cr_log_destory_ctx(cr_log_global_ctx)
#define cr_log_set_level(level)      cr_log_set_level_ctx(cr_log_global_ctx, level)
#define cr_log_scope_set(scope)      cr_log_scope_set_ctx(cr_log_global_ctx, scope)
#define cr_log_sink_add(level, sink) cr_log_sink_add_ctx(cr_log_global_ctx, level, sink)

// sink
#define cr_log_sink_text(...) cr_log_sink_text_new((cr_log_transport *[]) { __VA_ARGS__, nullptr })
cr_log_sink *cr_log_sink_text_new(cr_log_transport **transports);

// transport
cr_log_transport *cr_log_transport_fd(int fd);
cr_log_transport *cr_log_transport_file(const char *file, int flags);

#ifdef CR_LOG_TELEMETRY
#define cr_log_get_dropped() cr_log_get_dropped_ctx(cr_log_global_ctx)
uint64_t cr_log_get_dropped_ctx(cr_log_ctx *ctx);
#endif

/******************************  Implementation  ******************************/
#if defined(CR_LOG_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

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

// cr_types.h
#ifndef CR_TYPES_H

typedef int8_t    i8;
typedef uint8_t   u8;
typedef int16_t   i16;
typedef uint16_t  u16;
typedef int32_t   i32;
typedef uint32_t  u32;
typedef int64_t   i64;
typedef uint64_t  u64;
typedef float     f32;
typedef double    f64;
typedef bool      b8;
typedef size_t    usize;
typedef ptrdiff_t isize;

#endif

#ifndef CR_LOG_MAX_INSTANCES
#define CR_LOG_MAX_INSTANCES 16
#endif

#ifndef CR_LOG_ITEM_SIZE
#define CR_LOG_ITEM_SIZE (1 << 9)
#endif

#ifndef CR_LOG_ITEM_FIELDS
#define CR_LOG_ITEM_FIELDS 8
#endif

#ifndef CR_LOG_QUEUE_SIZE
#define CR_LOG_QUEUE_SIZE (1 << 12)
#endif

#ifndef CR_LOG_ITABLE_SIZE
#define CR_LOG_ITABLE_SIZE (1 << 8)
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS
#define CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS (1 << 7)
#endif

#ifndef CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF
#define CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF (1 << 10)
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

#define cr_log__likely(x)   __builtin_expect(!!(x), 1)
#define cr_log__unlikely(x) __builtin_expect(!!(x), 0)
#define cr_log__err(fmt, ...)                                                                                          \
    do {                                                                                                               \
        (void)fprintf(stderr, "error(%s:%s:%d)", __FILE__, __func__, __LINE__);                                        \
        (void)fprintf(stderr, (fmt), ##__VA_ARGS__);                                                                   \
    } while (0)

#define cr_log__iter_field(field_arr, label)                                                                           \
    for (cr_log_field * (label) = field_arr;                                                                           \
         (label) - (field_arr) < CR_LOG_ITEM_FIELDS && (label)->type != CR_LOG_TYPE_NONE;                              \
         (label)++)

#define atomic_load_relaxed(target)         atomic_load_explicit(target, memory_order_relaxed)
#define atomic_load_acquire(target)         atomic_load_explicit(target, memory_order_acquire)
#define atomic_store_relaxed(target, value) atomic_store_explicit(target, value, memory_order_relaxed)
#define atomic_store_release(target, value) atomic_store_explicit(target, value, memory_order_release)
#define atomic_cas(target, expected, desired)                                                                          \
    atomic_compare_exchange_weak_explicit(target, expected, desired, memory_order_relaxed, memory_order_relaxed)

/**********************
 * zone:private:types *
 **********************/

enum cr_log_type {
    /* NONE used as sentinal value */
    CR_LOG_TYPE_NONE = 0,
    CR_LOG_TYPE_U64,
    CR_LOG_TYPE_I64,
    CR_LOG_TYPE_BOOL,
    CR_LOG_TYPE_STRING,
};

struct cr_log_field {
    const char      *name;
    enum cr_log_type type;
    union {
        u64         u;
        i64         i;
        b8          b;
        const char *s;
    } value;
};

/* used for automatic buffer size calculation depending on various configuration macros */
struct cr_log_item_layout {
    alignas(CACHE_LINE_SIZE) atomic_size_t sequence;
    u8              level;
    const char     *message;
    struct timespec time;
    u32             line;
    const char     *filename;
    const char     *function;
    const char     *scope;
    i64             scope_id;
    cr_log_field    fields[CR_LOG_ITEM_FIELDS];
    char           *arena_ptr;
    char            arena[0];
};

static constexpr size_t cr_log__buffer_size = CR_LOG_ITEM_SIZE - (offsetof(struct cr_log_item_layout, arena));
static constexpr size_t cr_log__queue_size  = CR_LOG_QUEUE_SIZE;

struct cr_log_item {
    u8              level;
    const char     *message;
    struct timespec time;

    /* Caller Meta */
    u32         line;
    const char *filename;
    const char *function;

    /* scope_id used for transport, scope used by sinks */
    const char *scope;
    i64         scope_id;

    /* fmt and kv fields */
    cr_log_field fields[CR_LOG_ITEM_FIELDS];

    /* Arena used for safe string field transport */
    char *arena_ptr;
    char  arena[cr_log__buffer_size];
};

/* struct cr_log_capture
 * Captured when calling cr_log,
 * temporary till item is safely copied after claiming queue slot
 */
typedef struct cr_log_capture {
    u8              level;
    const char     *message;
    struct timespec time;

    u32         line;
    const char *filename;
    const char *function;

    const char   *scope;
    i64           scope_id;
    cr_log_field *fields;
} cr_log_capture;

typedef struct cr_log_queue {
    alignas(CACHE_LINE_SIZE) atomic_size_t write;
    alignas(CACHE_LINE_SIZE) atomic_size_t read;

    usize mask;
    sem_t items;

    struct {
        alignas(CACHE_LINE_SIZE) atomic_size_t sequence;
        struct cr_log_item event;
    } buffer[cr_log__queue_size];
} cr_log_queue;

static_assert(
    sizeof(struct {
        alignas(CACHE_LINE_SIZE) atomic_size_t sequence;
        struct cr_log_item                     event;
    }) == CR_LOG_ITEM_SIZE,
    "cr_log_item slot size mismatch");

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

typedef void (*cr_log_process_fn)(cr_log_sink *sink, cr_log_item *item);
struct cr_log_sink {
    struct cr_log_sink *next;
    cr_log_process_fn   process;
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

static cr_log_sink *cr_log__sink_new(cr_log_process_fn process, usize bsize, struct cr_log_transport **transports);
static void         cr_log__sink_free(cr_log_sink *sink);
static inline i32   cr_log__sink_flush(cr_log_sink *sink);

static inline i32 cr_log__sink_write(cr_log_sink *sink, const void *src, usize size);
static inline i32 cr_log__sink_str(cr_log_sink *sink, const char *str);
static i32        cr_log__sink_u64(cr_log_sink *sink, u64 value);
static i32        cr_log__sink_i64(cr_log_sink *sink, i64 value);
static inline i32 cr_log__sink_bool(cr_log_sink *sink, b8 value);

static inline char *cr_log__sink_reserve(cr_log_sink *sink, usize size);
static inline void  cr_log__sink_advance(cr_log_sink *sink, usize size);

// format item->message using format fields
static inline void cr_log__sink_message(cr_log_sink *sink, const cr_log_item *item);

// auto select appropriate cr_log__sink_* according field->type
static inline i32 cr_log__sink_value(cr_log_sink *sink, const cr_log_field *field);

static void cr_log__sink_text_process(cr_log_sink *sink, cr_log_item *item);

static int cr_log__transport_fd_write(cr_log_transport *transport, void *src, usize size);
static int cr_log__transport_fd_close(cr_log_transport *transport);

static int cr_log__transport_file_close(cr_log_transport *transport);

static int          cr_log__try_enqueue(cr_log_ctx *ctx, cr_log_capture *event);
static int          cr_log__enqueue(cr_log_ctx *ctx, cr_log_capture *event);
static inline char *cr_log__item_strdup(cr_log_item *item, const char *str);
static int          cr_log__item_copy(cr_log_item *target, const cr_log_capture *source);

static int   cr_log__try_dequeue(cr_log_ctx *ctx);
static void *cr_log__dequeue(void *arg);
static void  cr_log__consume(cr_log_ctx *ctx, struct cr_log_item *item);

/************************
 * zone:private:statics *
 ************************/

cr_log_ctx *cr_log_global_ctx;

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
alignas(CACHE_LINE_SIZE) atomic_uint_fast64_t cr_log__drop_count;
#endif

// monotonic instance id counter
static atomic_uint_fast16_t cr_log__instance_counter = 0;

// per thread scope id storage
// id is a index into the per-instance intern table
thread_local struct {
    uint16_t scope_id;
} cr_log__per_instance_scope[CR_LOG_MAX_INSTANCES];

static struct cr_log_transport_ops cr_log__fd_ops = {
    .write = cr_log__transport_fd_write,
    .close = cr_log__transport_fd_close,
};

static struct cr_log_transport_ops cr_log__file_ops = {
    .write = cr_log__transport_fd_write,
    .close = cr_log__transport_file_close,
};

/************************
 * zone:private:helpers *
 ************************/

// NOLINTBEGIN(readability-magic-numbers)
static const u64 cr_log__pow10_lut[] = {
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
cr_log__log10(u64 value)
{
    //! x  =>  floor(floor(log2(x)) * log10(2))
    u8 guess = ((63 - (u8)__builtin_clzll(value | 1ULL)) * 1233) >> 12;
    u8 next  = guess + (guess < 19);
    return guess + (value >= cr_log__pow10_lut[next]);
}

static inline u64
cr_log__abs(i64 value)
{
    u64 mask = (u64)value >> 63;
    return ((u64)value + mask) ^ mask;
}

#define FNV1A_64_PRIME  0x00000100000001b3ULL
#define FNV1A_64_OFFSET 0xcbf29ce484222325ULL

static inline uint64_t
cr_log__hash_string(const char *_key)
{
    u8 *key  = (u8 *)_key;
    u64 hash = FNV1A_64_OFFSET;
    while (*key) {
        hash ^= *key++;
        hash *= FNV1A_64_PRIME;
    }

    return hash;
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

    ctx->instance_id = atomic_fetch_add_explicit(&cr_log__instance_counter, 1, memory_order_relaxed);
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
    for (usize i = 0; i < cr_log__queue_size; i++) {
        atomic_init(&ctx->queue.buffer[i].sequence, i);
    }
    ctx->queue.mask = cr_log__queue_size - 1;
    atomic_init(&ctx->queue.read, 0);
    atomic_init(&ctx->queue.write, 0);
    sem_init(&ctx->queue.items, 0, 0);
    pthread_create(&ctx->consumer, NULL, cr_log__dequeue, ctx);

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

static inline char *
cr_log__item_strdup(cr_log_item *item, const char *str)
{
    usize len = strlen(str) + 1; // adjust for '\0'
    if (cr_log__likely(item->arena_ptr + len <= item->arena + cr_log__buffer_size)) {
        char *copy = memcpy(item->arena_ptr, str, len);
        item->arena_ptr += len;
        return copy;
    }
    return NULL;
}

static int
cr_log__item_copy(cr_log_item *target, const cr_log_capture *source)
{
    int err = 0;

    target->level    = source->level;
    target->message  = source->message;
    target->time     = source->time;
    target->line     = source->line;
    target->filename = source->filename;
    target->function = source->function;
    target->scope_id = source->scope_id;

    // init arena
    target->arena_ptr = target->arena;

    // copy fields
    for (usize i = 0; i < CR_LOG_ITEM_FIELDS; i++) {
        cr_log_field *source_field = source->fields + i;
        if (!source_field || source_field->type == CR_LOG_TYPE_NONE) {
            // sentinel value
            break;
        }

        cr_log_field *target_field = target->fields + i;
        target_field->name         = source_field->name;
        target_field->type         = source_field->type;
        target_field->value        = source_field->value;

        if (source_field->type == CR_LOG_TYPE_STRING && !err) {
            // safely copy string value (using arena)
            target_field->value.s = cr_log__item_strdup(target, source_field->value.s);
            if (target_field->value.s == nullptr) {
                err = 1;
            }
        }
    }

    return err;
}

static int
cr_log__try_enqueue(cr_log_ctx *ctx, cr_log_capture *event)
{
    for (;;) {
        usize write_pos = atomic_load_relaxed(&ctx->queue.write);
        usize idx       = write_pos & ctx->queue.mask;
        usize seq       = atomic_load_relaxed(&ctx->queue.buffer[idx].sequence);

        // check availablity
        if (write_pos - seq == 0) {
            // available, try cas
            if (atomic_cas(&ctx->queue.write, &write_pos, write_pos + 1)) {
                // claimed, write
                cr_log__item_copy(&ctx->queue.buffer[idx].event, event);

                // finish
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
cr_log__enqueue(cr_log_ctx *ctx, cr_log_capture *event)
{
    i32 backoff = 1;

    for (int i = 0; i < CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS; i++) {
        i32 ret = cr_log__try_enqueue(ctx, event);
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

static int
cr_log__try_dequeue(cr_log_ctx *ctx)
{
    usize read_pos = atomic_load_relaxed(&ctx->queue.read);
    usize idx      = read_pos & ctx->queue.mask;
    usize seq      = atomic_load_acquire(&ctx->queue.buffer[idx].sequence);
    isize diff     = (isize)(seq - (read_pos + 1));

    if (diff == 0) {
        // consume
        cr_log__consume(ctx, &ctx->queue.buffer[idx].event);

        // release
        atomic_store_relaxed(&ctx->queue.buffer[idx].sequence, read_pos + cr_log__queue_size);
        atomic_store_relaxed(&ctx->queue.read, read_pos + 1);

        return 0;
    }

    return -1;
}

void *
cr_log__dequeue(void *arg)
{
    cr_log_ctx *ctx = (cr_log_ctx *)arg;
    for (;;) {
        sem_wait(&ctx->queue.items);
        while (cr_log__try_dequeue(ctx) == 0) {
            if (sem_trywait(&ctx->queue.items) != 0) {
                break;
            }
        }

        // drain on shutdown
        if (!atomic_load_relaxed(&ctx->state)) {
            while (cr_log__try_dequeue(ctx) == 0) { }
            break;
        }
    }

    return NULL;
}

cr_log_field
cr_log_i64(i64 value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_I64, .value.i = value };
}

cr_log_field
cr_log_u64(u64 value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_U64, .value.u = value };
}

cr_log_field
cr_log_bool(bool value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_BOOL, .value.b = value };
}

cr_log_field
cr_log_str(const char *value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_STRING, .value.s = value };
}

inline cr_log_field
cr_log_kv(const char *key, cr_log_field value)
{
    return (cr_log_field) { .name = key, .type = value.type, .value = value.value };
}

inline cr_log_field
cr_log_pass(cr_log_field value)
{
    return value;
}

#ifdef CR_LOG_TELEMETRY
uint64_t
cr_log_get_dropped_ctx(cr_log_ctx *ctx)
{
    return atomic_load(&ctx->dropped);
}
#endif

void
cr_log_submit(
    cr_log_ctx   *ctx,
    cr_log_level  level,
    const char   *file,
    i32           line,
    const char   *func,
    const char   *message,
    cr_log_field *fields)
{
    // runtime purge
    if (level < ctx->level) {
        return;
    }
    cr_log_capture event = { .level    = level,
                             .time     = { 0 },
                             .filename = file,
                             .line     = (u32)line,
                             .function = func,
                             .scope_id = cr_log__per_instance_scope[ctx->instance_id].scope_id,
                             .message  = message,
                             .fields   = fields };

    clock_gettime(CLOCK_REALTIME_COARSE, &event.time);

    cr_log__enqueue(ctx, &event);
}

// * Scope
void
cr_log_scope_set_ctx(cr_log_ctx *ctx, const char *scope)
{
    auto     hash = cr_log__hash_string(scope);
    uint16_t idx  = hash & ctx->itable.mask;

    pthread_mutex_lock(&ctx->itable.write_lock);
    for (i32 i = 0; i < (CR_LOG_ITABLE_SIZE); i++) {
        auto entry = &ctx->itable.items[idx];

        // 0 is reserved for "default" scope
        if (idx != 0 && !entry->used) {
            entry->key = strdup(scope);
            if (!entry->key) {
                cr_log__per_instance_scope[ctx->instance_id].scope_id = 0;
                goto cleanup;
            }

            entry->used                                           = true;
            entry->hash                                           = hash;
            cr_log__per_instance_scope[ctx->instance_id].scope_id = idx;
            goto cleanup;
        } else if (entry->hash == hash && strcmp(entry->key, scope) == 0) {
            cr_log__per_instance_scope[ctx->instance_id].scope_id = idx;
            goto cleanup;
        } else {
            idx = (idx + 1) & ctx->itable.mask;
        }
    }
    cr_log__per_instance_scope[ctx->instance_id].scope_id = 0;

cleanup:
    pthread_mutex_unlock(&ctx->itable.write_lock);
}

static inline const char *
cr_log__scope_get(cr_log_ctx *ctx, int64_t sid)
{
    if (sid == -1) {
        return "";
    }

    return ctx->itable.items[sid].key;
}

void
cr_log__consume(cr_log_ctx *ctx, struct cr_log_item *item)
{
    item->scope = cr_log__scope_get(ctx, item->scope_id);

    cr_log_sink *sink = ctx->sinks_head;
    while (sink != NULL) {
        if (sink->level <= item->level) {
            sink->process(sink, item);
        }
        sink = sink->next;
    }
}

static cr_log_sink *
cr_log__sink_new(cr_log_process_fn process, usize bsize, struct cr_log_transport *transports[])
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

    if (cr_log__likely(sink->bpos + size <= sink->bsize)) {
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

static inline i32
cr_log__sink_write(cr_log_sink *sink, const void *src, usize size)
{
    char *dst = cr_log__sink_reserve(sink, size);
    if (cr_log__likely(dst)) {
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

static inline i32
cr_log__sink_str(cr_log_sink *sink, const char *str)
{
    return cr_log__sink_write(sink, str, strlen(str));
}

// NOLINTBEGIN(readability-magic-numbers)
static const char cr_log__digit_lut[] = "00010203040506070809"
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
    u8    len     = cr_log__log10(value) + 1;
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
        idx[0] = cr_log__digit_lut[rem];
        idx[1] = cr_log__digit_lut[rem + 1];
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
    u64  uvalue = cr_log__abs(value);
    // adjust to account for fast_log10 flooring and sign bit
    u8    len     = cr_log__log10(uvalue) + 1 + (value < 0);
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
        idx[0] = cr_log__digit_lut[rem];
        idx[1] = cr_log__digit_lut[rem + 1];
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

static inline i32
cr_log__sink_bool(cr_log_sink *sink, b8 value)
{
    if (value) {
        cr_log__sink_write(sink, "true", 4);
    } else {
        cr_log__sink_write(sink, "false", 5);
    }

    return 0;
}

// NOLINTEND(readability-magic-numbers)

static inline i32
cr_log__sink_value(cr_log_sink *sink, const cr_log_field *field)
{
    switch (field->type) {
    case CR_LOG_TYPE_U64:
        return cr_log__sink_u64(sink, field->value.u);
    case CR_LOG_TYPE_I64:
        return cr_log__sink_i64(sink, field->value.i);
    case CR_LOG_TYPE_BOOL:
        return cr_log__sink_bool(sink, field->value.b);
    case CR_LOG_TYPE_STRING:
        return cr_log__sink_str(sink, field->value.s);

    default:
        return -1;
    }
}

static inline void
cr_log__sink_message(cr_log_sink *sink, const cr_log_item *item)
{
    usize len = strlen(item->message);

    // current selected substring
    usize start = 0;
    usize end   = 0;

    const cr_log_field *field = item->fields;

    while (end < len) {
        if (item->message[end] != '{') {
            end++;
            continue;
        }

        // found '{', find possible '}'
        if (end + 1 < len && item->message[end + 1] == '}') {
            // write till '{'
            cr_log__sink_write(sink, item->message + start, end - start);

            // select next format field if possible
            if (field - item->fields < CR_LOG_ITEM_FIELDS && field->name != nullptr) {
                while (field - item->fields < CR_LOG_ITEM_FIELDS && field->name != nullptr) {
                    field++;
                }
            }

            cr_log__sink_value(sink, field);
            field++;

            // skip {} and advance
            end += 2;
            start = end;
        }
    }

    // write remaining, if any
    if (start < end) {
        cr_log__sink_write(sink, item->message + start, end - start);
    }
}

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

    // print formatted message
    cr_log__sink_message(sink, item);

    // key value pairs
    cr_log__iter_field(item->fields, field)
    {
        // filter kv pairs
        if (field->name != nullptr) {
            cr_log__sink_write(sink, " ", 1);
            cr_log__sink_str(sink, field->name);
            cr_log__sink_write(sink, "=", 1);
            cr_log__sink_value(sink, field);
        }
    }

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
    transport->base.ops = &cr_log__fd_ops;
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
    transport->ops              = &cr_log__file_ops;
    return transport;
}

static int
cr_log__transport_file_close(cr_log_transport *transport)
{
    close(((struct cr_log_transport_fd *)transport)->fd);
    return 0;
}

#endif // CR_LOG_IMPL
#endif // CR_LOG_H

/*******************************************************************************

- Compile time options

    CR_LOG_PURGE_LEVEL (default: 0 aka CR_LOG_LEVEL_TRACE)
        Set this to appropriate CR_LOG_LEVEL_* to purge log function calls.
        Any log of a lower level is replaced by a (void(0)).
        Therefore, no runtime overhead.

    CR_LOG_MAX_INSTANCES (default: 16)
        This controls the upper limit on how many context can be created during app lifetime.
        This is necessary as this sets the size of how many scope_id to value any thread can handle.
        A value of 16 means, there can be total 16 context (including global one).
        Internally each context gets a monotonic context id,
        Which is used to look up what scope is set for any context per thread.
        Higher value will increase memory overhead per thread.

    CR_LOG_ITEM_SIZE (default: 2^9 aka 512)
        Controls the size of individual items in queue.

    CR_LOG_ITEM_FIELDS (default: 8)
        Max capacity of each log item to hold fields.
        Fields can be either format parameter or key-value pair.
        Excess fields will be dropped.

    CR_LOG_QUEUE_SIZE (default: 2^12 aka 4096)
        Size of the internal queue used to asyncronously dispatch log calls.
        Only tune this if you are working under extreme multithreadedthread load.
        The default value is usually more than enough.
        Profile using CR_LOG_TELEMETRY to see dropped items.

    CR_LOG_ITABLE_SIZE (default: 2^8 aka 256)
        Used to set the size of intern table inside all the contexts.
        Only nessecary if you set scopes frequently (default size is 256).
        NOTE: max safe value is 1<<16 (limited by the capacity of uint16_t)

    CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS (default: 2^7 aka 128)
        Max attempts to enqueue an item before dropping it.

    CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF (default: 2^10 aka 1024)
        Used to clamp backoff spin timing.

- Documentation
    To be added.

- Examples
    To be added.

- MIT License
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

*******************************************************************************/
