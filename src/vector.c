#include "vector.h"

#include <stddef.h>
#include <stdlib.h>

static inline size_t
max_sz(size_t size1, size_t size2)
{
    return size1 > size2 ? size1 : size2;
}

void
vector_new_(void** ptr, size_t item_size, vector_options_t opts)
{
    // apply defaults
    if (!opts.initial_capacity) {
        opts.initial_capacity = vector_min_capacity;
    }

    vector_header_t* header = malloc(sizeof(vector_header_t) + (opts.initial_capacity * item_size));
    if (!header) {
        return;
    }

    header->size     = 0;
    header->capacity = opts.initial_capacity;

    *ptr = header + 1;
}

void
vector_reserve_(void** ptr, size_t item_size, size_t count)
{
    vector_header_t* header = _vector_header(*ptr);
    if (header->capacity < count) {
        size_t           new_capacity = max_sz(count, header->capacity * 2);
        vector_header_t* new_vec      = realloc(header, sizeof(vector_header_t) + (new_capacity * item_size));
        if (!new_vec) {
            return;
        }

        header           = new_vec;
        header->capacity = new_capacity;
        *ptr             = header + 1;
    }
}
