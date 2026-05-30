/*
    cr_types.h - v0.0.1 - Type Conventions

    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/corrosive

    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob

    For other informations, see the end of this file.
 */

#ifndef CR_TYPES_H
#define CR_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef float  f32;
typedef double f64;

typedef uint8_t byte;

typedef size_t    usize;
typedef ptrdiff_t isize;

typedef _Bool b8;

#endif /* CR_TYPES_H */
