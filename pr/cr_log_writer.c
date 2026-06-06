#include "../cr_types.h"

#include <bits/time.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef struct writer_t {

    i32 fd;

    // buffer
    usize bsize;
    char *buffer;
    usize bpos;

} writer_t;

extern writer_t *writer_create(int target_fd, usize bsize);
extern void      writer_write(writer_t *writer, const void *src, usize size);
extern void      writer_write_i64(writer_t *writer, i64 value);
extern void      writer_flush(writer_t *writer);
extern void      writer_destroy(writer_t *writer);

// internal
static inline char *writer__reserve(writer_t *writer, usize size);
static inline void  writer__advance(writer_t *writer, usize size);

writer_t *
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

/*
 * Reserves enough space in the buffer to write `size` bytes.
 * returns a pointer to the reserved space.
 * caller must make sure to write exactly `size` bytes to this location.
 * if the buffer is not large enough, it returns NULL.
 * it means only writethrough is possible
 */
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

void
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
                return;
            }
            written += (usize)current_write;
        }
    }
}

inline void
writer_flush(writer_t *writer)
{
    usize idx = 0;
    while (idx < writer->bpos) {
        isize written = write(writer->fd, writer->buffer + idx, writer->bpos - idx);
        if (written < 0) {
            perror("write");
            return;
        }
        idx += (usize)written;
    }
    writer->bpos = 0;
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
//!		 ^^^^^
static inline u8
fast_log10(u64 value)
{
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
void
writer_write_i64(writer_t *writer, i64 value)
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

    if (ibuffer == stack_buffer) {
        writer_write(writer, idx, (usize)(ibuffer + len - idx));
    } else {
        writer__advance(writer, len);
    }
}

// NOLINTEND(readability-magic-numbers)
