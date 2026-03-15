#include "vector.h"

#include <stddef.h>
#include <stdlib.h>

void
vector_new_(void** ptr, size_t item_size, vector_options_t opts)
{
    vector_header_t* header = malloc(sizeof(vector_header_t) + (opts.initial_capacity * item_size));
    if (!header) {

        return;
    }

    // apply defaults
    if (!opts.initial_capacity) {
        opts.initial_capacity = vector_min_capacity;
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
        vector_header_t* new_vec = realloc(header, sizeof(vector_header_t) + (count * item_size * 2));
        if (!new_vec) {
            return;
        }

        header           = new_vec;
        header->capacity = header->capacity * 2;
        *ptr             = header + 1;
    }
}
