/*
    cr_vector.h - v0.1.0 - dynamically sized array (vector)

    Author:   Praise Jacob <iampraisejacob@gmail.com>
    Repo:     https://github.com/felix-kyun/corrosive

    SPDX-License-Identifier: MIT
    Copyright (c) 2026 Praise Jacob

    For other informations, see the end of this file.
 */

#ifndef CR_VECTOR_H
#define CR_VECTOR_H

#include <stddef.h>
#include <stdlib.h>

// config
#define vector_min_capacity 16

typedef struct vector_options_t {
    size_t initial_capacity;
} vector_options_t;

typedef struct vector_header_t {
    size_t size;
    size_t capacity;
} vector_header_t;

#define _vector_header(ptr)    ((vector_header_t *)(ptr) - 1)
#define _vector_item_size(ptr) (sizeof(*(ptr)))
#define _ptr_location(ptr)     ((void **)&(ptr))

#define vector_new(ptr, ...)                                                                                           \
    (vector_new_(_ptr_location(ptr), _vector_item_size(ptr), (vector_options_t) { __VA_ARGS__ }))
#define vector_free(ptr)                (free(_vector_header(ptr)), (ptr) = NULL)
#define vector_size(ptr)                (_vector_header(ptr)->size)
#define vector_set_capacity(ptr, count) (vector_reserve_(_ptr_location(ptr), _vector_item_size(ptr), count))
#define vector_reserve(ptr, count)      (vector_set_capacity(ptr, vector_size(ptr) + (count)));
#define vector_push_back(vec, item)     (vector_reserve(ptr, 1), (vec)[vector_size(vec)++] = (item))
#define vector_pop_back(vec)            ((vec)[--vector_size(vec)])
#define vector_peek_back(vec)           ((vec)[vector_size(vec) - 1])

void vector_new_(void **ptr, size_t item_size, vector_options_t opts);
void vector_reserve_(void **ptr, size_t item_size, size_t count);

#if defined(CR_VECTOR_IMPL) || defined(CORROSIVE_IMPLEMENTATION)

static inline size_t
max_sz(size_t size1, size_t size2)
{
    return size1 > size2 ? size1 : size2;
}

void
vector_new_(void **ptr, size_t item_size, vector_options_t opts)
{
    // apply defaults
    if (!opts.initial_capacity) {
        opts.initial_capacity = vector_min_capacity;
    }

    vector_header_t *header = malloc(sizeof(vector_header_t) + (opts.initial_capacity * item_size));
    if (!header) {
        return;
    }

    header->size     = 0;
    header->capacity = opts.initial_capacity;

    *ptr = header + 1;
}

void
vector_reserve_(void **ptr, size_t item_size, size_t count)
{
    vector_header_t *header = _vector_header(*ptr);
    if (header->capacity < count) {
        size_t           new_capacity = max_sz(count, header->capacity * 2);
        vector_header_t *new_vec      = realloc(header, sizeof(vector_header_t) + (new_capacity * item_size));
        if (!new_vec) {
            return;
        }

        header           = new_vec;
        header->capacity = new_capacity;
        *ptr             = header + 1;
    }
}

#endif // CR_VECTOR_IMPL
#endif // CR_VECTOR_H

/*
This is a single header library that provides a dynamic array implementation.
This is part of the Corrosive library.

To use this library, do this in *one* of your source files:
    #define CR_VECTOR_IMPL
    #include "cr_vector.h"

Table Of Contents
    Compile time options
    Documentation
    Examples
    License
    Credits

Compile time options
        vector_min_capacity <number>
                The minimum capacity of the vector when created.
                if the user passes a capacity less than this, it will be overridden to this value.

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
