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

#ifndef CR_LOG_PURGE_LEVEL
#define CR_LOG_PURGE_LEVEL CR_LOG_LEVEL_TRACE
#endif

#ifndef CR_LOG_SINK_FILE_BUFFER
#define CR_LOG_SINK_FILE_BUFFER 10240
#endif

#ifndef CR_LOG_SCOPE_INTERN_TABLE_SIZE_POWER
#define CR_LOG_SCOPE_INTERN_TABLE_SIZE_POWER 8
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
#define CR_LOG_QUEUE_MAX_ENQUEUE_BACKOFF 1024
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

#ifdef CR_LOG_TELEMETRY
uint64_t cr_log_get_dropped(void);
#endif

[[gnu::hot, gnu::format(__printf__, 5, 6)]]
void cr_log(cr_log_level_t level, const char *file, int line, const char *func, const char *fmt, ...);
void cr_log_set_level(cr_log_level_t level);

// * scope
extern thread_local int64_t scope_id;
void                        cr_log_scope_set(const char *scope);

// * sinks
typedef struct cr_log_item_t cr_log_item_t;
typedef struct cr_log_sink_t {
    cr_log_level_t level;
    void          *state;
    void (*process)(void *sink_state, const cr_log_item_t *item);
    void (*flush)(void *sink_state);
    void (*free)(void *sink_state);
} cr_log_sink_t;

void cr_log_sink_add(cr_log_level_t level, cr_log_sink_t sink);

#define cr_log_sink_default() cr_log_sink_fd_new(.fd = STDERR_FILENO, .level = CR_LOG_LEVEL_TRACE, .bsize = 0)

// ** FD sink
struct cr_log_sink_fd_config_t {
    int    fd;
    size_t bsize;
};
cr_log_sink_t cr_log__sink_fd_new(struct cr_log_sink_fd_config_t config);
#define cr_log_sink_fd(...) cr_log__sink_fd_new((struct cr_log_sink_fd_config_t) { __VA_ARGS__ })

// ** file sink
typedef struct cr_log_sink_file_config_t {
    const char *target;
    bool        truncate;
    size_t      bsize;
} cr_log_sink_file_config_t;

cr_log_sink_t cr_log__sink_file_new(struct cr_log_sink_file_config_t config);
#define cr_log_sink_file(...) cr_log__sink_file_new((struct cr_log_sink_file_config_t) { __VA_ARGS__ })

#if defined(CR_LOG_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

/* Implementation */

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

// clang-format off
thread_local int64_t    scope_id   = 0;
static constexpr size_t queue_size = 1 << CR_LOG_QUEUE_SIZE_POWER;
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

typedef struct cr_log_item_t {
    u8              level;
    u32             line;
    const char     *filename;
    const char     *function;
    const char     *scope;
    struct timespec time;
    i64             scope_id;
    char            buffer[buffer_size];
} cr_log_item_t;

struct item_t {
    alignas(CACHE_LINE_SIZE) atomic_size_t sequence;
    struct cr_log_item_t meta;
};

struct queue_t {
    alignas(CACHE_LINE_SIZE) atomic_size_t write;
    char          pad1[CACHE_LINE_SIZE - sizeof(atomic_size_t)];
    atomic_size_t read;
    char          pad2[CACHE_LINE_SIZE - sizeof(atomic_size_t)];

    usize       mask;
    sem_t       items;
    pthread_t   consumer_thread;
    atomic_bool shutdown;

    struct item_t buffer[queue_size];
} queue;

static int   enqueue_(struct cr_log_item_t *meta);
static int   enqueue(struct cr_log_item_t meta);
static int   try_dequeue(void);
static void *dequeue(void *arg);
static void  queue_consumer(struct cr_log_item_t *item);

// intern table
static struct intern_table_t {
    pthread_rwlock_t lock;
    struct {
        u64   hash;
        char *key;
        bool  used;
    } items[1 << CR_LOG_SCOPE_INTERN_TABLE_SIZE_POWER];
    uint64_t mask;
} itable;

static inline u64 hash_string(const char *_key);

// writer interface
typedef struct writer_t {
    i32 fd;

    // buffer
    usize bsize;
    char *buffer;
    usize bpos;
} writer_t;

static writer_t *writer_create(int target_fd, usize bsize);
static i32       writer_flush(writer_t *writer);
static void      writer_destroy(writer_t *writer);

static i32 writer_write(writer_t *writer, const void *src, usize size);
static i32 writer_str(writer_t *writer, const char *str);
static i32 writer_i64(writer_t *writer, i64 value);
static i32 writer_u64(writer_t *writer, u64 value);

static inline char *writer__reserve(writer_t *writer, usize size);
static inline void  writer__advance(writer_t *writer, usize size);

//! sink using fd should inherit sink_fd_base_t
typedef struct sink_fd_base_t {
    writer_t *writer;
} sink_fd_base_t;

// FD Sink
typedef sink_fd_base_t cr_log_sink_fd_state_t;
void                   cr_log__sink_fd_flush(void *_state);
void                   cr_log__sink_fd_process(void *_state, const cr_log_item_t *item);
void                   cr_log__sink_fd_free(void *_state);

// File Sink
typedef struct cr_log_sink_file_state_t {
    sink_fd_base_t                   base;
    struct cr_log_sink_file_config_t config;
} cr_log_sink_file_state_t;

// uses sink_fd methods for flush and process
void cr_log__sink_file_free(void *sink_state);

static_assert(sizeof(struct item_t) == CR_LOG_QUEUE_ITEM_SIZE, "item too large");
static_assert(alignof(struct item_t) == CACHE_LINE_SIZE, "alignment broken");

int
enqueue_(struct cr_log_item_t *meta)
{
    for (;;) {
        usize write_pos = atomic_load_relaxed(&queue.write);
        usize idx       = write_pos & queue.mask;
        usize seq       = atomic_load_relaxed(&queue.buffer[idx].sequence);

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
    i32 backoff = 1;

    for (int i = 0; i < CR_LOG_QUEUE_MAX_ENQUEUE_ATTEMPTS; i++) {
        i32 ret = enqueue_(&meta);
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
    atomic_fetch_add(&drop_count, 1);
#endif
    return -1;
}

int
try_dequeue(void)
{
    usize read_pos = atomic_load_relaxed(&queue.read);
    usize idx      = read_pos & queue.mask;
    usize seq      = atomic_load_acquire(&queue.buffer[idx].sequence);
    isize diff     = (isize)(seq - (read_pos + 1));

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

        if (atomic_load_relaxed(&queue.shutdown)) {
            while (try_dequeue() == 0) { }
            break;
        }
    }

    return NULL;
}

static struct {
    cr_log_level_t level;
    cr_log_sink_t  sinks[CR_LOG_SINK_LIMIT];
    usize          sink_count;
} logger_state = { 0 };

void
cr_log_init(void)
{
    logger_state.level = CR_LOG_LEVEL_INFO;
    for (usize i = 0; i < queue_size; i++) {
        atomic_init(&queue.buffer[i].sequence, i);
    }
    queue.mask = queue_size - 1;
    atomic_init(&queue.read, 0);
    atomic_init(&queue.write, 0);
    atomic_init(&queue.shutdown, false);
    sem_init(&queue.items, 0, 0);

#ifdef CR_LOG_TELEMETRY
    atomic_init(&drop_count, 0);
#endif

    // intern table
    itable.mask = (1 << CR_LOG_SCOPE_INTERN_TABLE_SIZE_POWER) - 1;
    pthread_rwlock_init(&itable.lock, NULL);

    // spawn consumer thread
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
    for (usize i = 0; i < logger_state.sink_count; i++) {
        logger_state.sinks[i].flush(logger_state.sinks[i].state);
    }
}

void
cr_log_free(void)
{
    // signal and wait for consumer thread to finish
    atomic_store_relaxed(&queue.shutdown, true);
    sem_post(&queue.items);
    pthread_join(queue.consumer_thread, NULL);
    sem_destroy(&queue.items);

    // free intern table
    pthread_rwlock_wrlock(&itable.lock);
    for (i32 i = 0; i < (1 << CR_LOG_SCOPE_INTERN_TABLE_SIZE_POWER); i++) {
        if (itable.items[i].used) {
            free(itable.items[i].key);
            itable.items[i].used = false;
        }
    }
    pthread_rwlock_unlock(&itable.lock);
    pthread_rwlock_destroy(&itable.lock);

    // free sinks
    for (usize i = 0; i < logger_state.sink_count; i++) {
        logger_state.sinks[i].free(logger_state.sinks[i].state);
    }
}

#ifdef CR_LOG_TELEMETRY
uint64_t
cr_log_get_dropped()
{
    return atomic_load(&drop_count);
}
#endif

void
cr_log(cr_log_level_t level, const char *file, i32 line, const char *func, const char *fmt, ...)
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
        .line      = (u32)line,
        .function  = func,
        .scope_id = scope_id
    };
    // clang-format on

    clock_gettime(CLOCK_REALTIME_COARSE, &event.time);

    va_list args;
    va_start(args, fmt);
    (void)vsnprintf(event.buffer, buffer_size, fmt, args);
    va_end(args);

    enqueue(event);
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
cr_log_scope_set(const char *scope)
{
    auto hash       = hash_string(scope);
    u64  idx        = hash & itable.mask;
    bool write_mode = false;
    pthread_rwlock_rdlock(&itable.lock);
    for (i32 i = 0; i < (1 << CR_LOG_SCOPE_INTERN_TABLE_SIZE_POWER); i++) {
        auto entry = &itable.items[idx];
        if (!entry->used) {
            if (!write_mode) {
                pthread_rwlock_unlock(&itable.lock);
                // upgrade to write lock and recheck
                pthread_rwlock_wrlock(&itable.lock);
                write_mode = true;
                idx        = hash & itable.mask;
                i          = -1;
                continue;
            }

            entry->key = strdup(scope);
            if (!entry->key) {
                scope_id = -1;
                goto cleanup;
            }

            entry->hash = hash;
            entry->used = true;
            scope_id    = (int64_t)idx;
            goto cleanup;
        } else if (entry->hash == hash && strcmp(entry->key, scope) == 0) {
            scope_id = (int64_t)idx;
            goto cleanup;
        } else {
            idx = (idx + 1) & itable.mask;
        }
    }
    // ignore scope at this level as we ran out of space
    scope_id = -1;

cleanup:
    pthread_rwlock_unlock(&itable.lock);
}

// * Writer
static writer_t *
writer_create(i32 target_fd, usize bsize)
{
    if (target_fd < 0) {
        return NULL;
    }

    writer_t *writer = malloc(sizeof(writer_t));
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
writer__reserve(writer_t *writer, usize size)
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
writer__advance(writer_t *writer, usize size)
{
    writer->bpos += size;
}

i32
writer_write(writer_t *writer, const void *src, usize size)
{
    char *dst = writer__reserve(writer, size);
    if (likely(dst)) {
        memcpy(dst, src, size);
        writer__advance(writer, size);
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
writer_str(writer_t *writer, const char *str)
{
    return writer_write(writer, str, strlen(str));
}

inline i32
writer_flush(writer_t *writer)
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
writer_destroy(writer_t *writer)
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
writer_i64(writer_t *writer, i64 value)
{
    char stack_buffer[32];
    u64  uvalue = fast_abs(value);
    // adjust to account for fast_log10 flooring and sign bit
    u8    len     = fast_log10(uvalue) + 1 + (value < 0);
    char *ibuffer = writer__reserve(writer, len);

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

    writer__advance(writer, len);
    return 0;
}

i32
writer_u64(writer_t *writer, u64 value)
{
    char stack_buffer[32];
    // adjust to account for fast_log10 flooring
    u8    len     = fast_log10(value) + 1;
    char *ibuffer = writer__reserve(writer, len);

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

    writer__advance(writer, len);
    return 0;
}

// NOLINTEND(readability-magic-numbers)

// * Sinks
void
cr_log_sink_add(cr_log_level_t level, cr_log_sink_t sink)
{
    if (logger_state.sink_count < CR_LOG_SINK_LIMIT) {
        sink.level                                    = level;
        logger_state.sinks[logger_state.sink_count++] = sink;
    }
}

cr_log_sink_t
cr_log__sink_fd_new(struct cr_log_sink_fd_config_t config)
{
    cr_log_sink_fd_state_t *state = malloc(sizeof(cr_log_sink_fd_state_t));
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
    return (cr_log_sink_t) { //
                             .state   = state,
                             .process = cr_log__sink_fd_process,
                             .flush   = cr_log__sink_fd_flush,
                             .free    = cr_log__sink_fd_free
    };
}

void
cr_log__sink_fd_flush(void *_state)
{
    auto state = (sink_fd_base_t *)_state;
    writer_flush(state->writer);
}

void
cr_log__sink_fd_process(void *_state, const cr_log_item_t *item)
{
    auto state  = (sink_fd_base_t *)_state;
    auto writer = state->writer;

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
}

void
cr_log__sink_fd_free(void *_state)
{
    auto state = (sink_fd_base_t *)_state;
    cr_log__sink_fd_flush(state);
    writer_destroy(state->writer);
    free(state);
}

cr_log_sink_t
cr_log__sink_file_new(struct cr_log_sink_file_config_t config)
{
    cr_log_sink_file_state_t *state = malloc(sizeof(cr_log_sink_file_state_t));
    if (!state) {
        perror("(malloc) file sink state allocation failed");
        return (cr_log_sink_t) { 0 };
    }

    int fd   = STDERR_FILENO;
    int mode = O_WRONLY | O_CREAT;
    if (config.truncate) {
        mode |= O_TRUNC;
    }
    if (config.target != nullptr) {
        fd = open(config.target, mode, 0644);
    } else {
        err("fallback to stderr");
    }

    state->config = config;
    state->base   = (sink_fd_base_t) {
        .writer = writer_create(fd, config.bsize),
    };

    return (cr_log_sink_t) {
        .state   = state,
        .process = cr_log__sink_fd_process,
        .flush   = cr_log__sink_fd_flush,
        .free    = cr_log__sink_file_free,
    };
}

void
cr_log__sink_file_free(void *sink_state)
{
    struct cr_log_sink_file_state_t *state = sink_state;
    int                              fd    = state->base.writer->fd;
    cr_log__sink_fd_flush(state);
    writer_destroy(state->base.writer);
    close(fd);
    free(state);
}

// consumer
static inline const char *
cr_log__scope_get(int64_t sid)
{
    if (sid == -1) {
        return "";
    }
    pthread_rwlock_rdlock(&itable.lock);
    const char *result = itable.items[sid].key;
    pthread_rwlock_unlock(&itable.lock);
    return result;
}

void
queue_consumer(struct cr_log_item_t *item)
{
    item->scope = cr_log__scope_get(item->scope_id);
    for (usize i = 0; i < logger_state.sink_count; i++) {
        cr_log_sink_t sink = logger_state.sinks[i];

        if (sink.level <= item->level) {
            sink.process(sink.state, item);
        }
    }
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
