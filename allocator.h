/*
 * allocator.h : Custom memory allocator with automtaic cleanup before program
 * termination
 *
 * Provides malloc-like functionality with thread safety. Memory is
 * automatically freed at program termination if not freed explicitly
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

/*
 * @brief Allocates a block of memory
 *
 * @param size Number of bytes to allocate
 *
 * @return Pointer to allocated memory on success, NULL on failiure
 */
void *malloc_a(size_t size);

/*
 * @brief Explicitly free a previously allocated block of memory
 *
 * @param block Pointer to a memory block to free
 */
void free_a(void *block);

/*
 * @brief Allocated and zero-initializes a block of memory
 *
 * @param num Number of elements
 * @param nsize Size of each element in bytes
 *
 * @return Pointer to zero-initialized allocated memory on success, NULL on
 * failiure
 */
void *calloc_a(size_t num, size_t nsize);

/*
 * @brief Resizes a previously allocated block of memory
 *
 * @param block Pointer to previously allocated memory
 * @param size New size in bytes
 *
 * @return Pointer to resized memory on success, NULL on failure
 */
void *realloc_a(void *block, size_t size);

#endif // !ALLOCATOR_H
