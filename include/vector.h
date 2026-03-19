#ifndef FELIX_VECTOR_H
#define FELIX_VECTOR_H

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

#define _vector_header(ptr)    ((vector_header_t*)(ptr) - 1)
#define _vector_item_size(ptr) (sizeof(*(ptr)))
#define _ptr_location(ptr)     ((void**)&(ptr))

#define vector_new(ptr, ...)                                                                                           \
    (vector_new_(_ptr_location(ptr), _vector_item_size(ptr), (vector_options_t) { __VA_ARGS__ }))
#define vector_free(ptr)            (free(_vector_header(ptr)), (ptr) = NULL)
#define vector_size(ptr)            (_vector_header(ptr)->size)
#define vector_reserve(ptr, count)  (vector_reserve_(_ptr_location(ptr), _vector_item_size(ptr), count))
#define vector_push_back(vec, item) (vector_reserve(vec, vector_size(vec) + 1), (vec)[vector_size(vec)++] = (item))
#define vector_pop_back(vec)        ((vec)[--vector_size(vec)])
#define vector_peek_back(vec)       ((vec)[vector_size(vec) - 1])

void vector_new_(void** ptr, size_t item_size, vector_options_t opts);
void vector_reserve_(void** ptr, size_t item_size, size_t count);

#endif //  FELIX_VECTOR_H
