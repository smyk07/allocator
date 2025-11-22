#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

void *malloc_a(size_t size);

void *calloc_a(size_t num, size_t nsize);

void *realloc_a(void *block, size_t size);

void free_a(void *block);

#endif // !ALLOCATOR_H
