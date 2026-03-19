#include "vector.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int
main(void)
{
    int* data = NULL;

    // should create and assign memory
    vector_new(data, .initial_capacity = 10);
    assert(data != NULL);

    // should push back and be able to peek back
    vector_push_back(data, 10);
    assert(vector_peek_back(data) == 10);

    vector_push_back(data, 20);
    assert(vector_peek_back(data) == 20);

    // should return the correct size
    assert(vector_size(data) == 2);

    // should pop back and return the correct value
    int popped = vector_pop_back(data);
    assert(popped == 20);

    // should return the correct size after pop back
    assert(vector_size(data) == 1);

    // should free memory and set pointer to NULL
    vector_free(data);
    assert(data == NULL);

    printf("vector tests passed\n");
    return 0;
}
