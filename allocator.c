#include "allocator.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

/*
 * @typedef ALIGN
 * @brief Allignment type for ensuring proper memory allignment.
 */
typedef char ALIGN[sizeof(max_align_t)];

/*
 * @union header_t
 * @brief memory block metadata header with allignment.
 *
 * Each block of memory is preceeded by this header, which tracks size,
 * allocation status, and links to succeeding blocks.
 *
 * Structure:
 *
 *    +--------------+------------------------+
 *    | header_t     | memory block           |
 *    +--------------+------------------------+
 *
 *    ^              ^
 *    |              |
 *    start          returned ptr
 */
typedef union header {
  struct {
    /* Size of the allocated memory block in bytes excluding the header */
    size_t size;

    /* Free status flag */
    unsigned is_free;

    /* Pointer to the next block in the free list */
    union header *next;

    /* Pointer to the next block in the allocation tracking list */
    union header *next_alloc;
  } s;
  ALIGN stub;
} header_t;

/*
 *  Global mutex for access to allocator data structures
 */
static pthread_mutex_t global_malloc_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Head of the allocation tracking list for automatic cleanup
 */
static header_t *alloc_list_head = NULL;

/*
 * Head and tail of the main memory block linked list
 */
static header_t *head;
static header_t *tail;

/*
 * @brief Searches the free list for a block large enough for the given size
 *
 * @param size Minimum size required in bytes
 *
 * @return Pointer to a suitable free block, or NULL
 */
static header_t *get_free_block(size_t size) {
  header_t *curr = head;
  while (curr) {
    if (curr->s.is_free && curr->s.size >= size)
      return curr;
    curr = curr->s.next;
  }
  return NULL;
}

void *malloc_a(size_t size) {
  size_t total_size;
  void *block;
  header_t *header;

  if (!size)
    return NULL;

  if (size > SIZE_MAX - sizeof(max_align_t) + 1) {
    return NULL;
  }

  size_t aligned_size =
      (size + sizeof(max_align_t) - 1) & ~(sizeof(max_align_t) - 1);

  if (aligned_size > SIZE_MAX - sizeof(header_t)) {
    return NULL;
  }

  pthread_mutex_lock(&global_malloc_lock);
  header = get_free_block(aligned_size);
  if (header) {
    header->s.is_free = 0;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void *)(header + 1);
  }

  total_size = sizeof(header_t) + aligned_size;

  block = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (block == MAP_FAILED) {
    pthread_mutex_unlock(&global_malloc_lock);
    return NULL;
  }

  header = block;
  header->s.size = aligned_size;
  header->s.is_free = 0;
  header->s.next = NULL;

  if (!head)
    head = header;
  if (tail)
    tail->s.next = header;
  tail = header;

  header->s.next_alloc = alloc_list_head;
  alloc_list_head = header;

  pthread_mutex_unlock(&global_malloc_lock);

  return (void *)(header + 1);
}

void free_a(void *block) {
  header_t *header, *tmp, *prev = NULL;

  if (!block)
    return;

  pthread_mutex_lock(&global_malloc_lock);
  header = (header_t *)block - 1;

  tmp = alloc_list_head;
  while (tmp) {
    if (tmp == header) {
      if (prev) {
        prev->s.next_alloc = tmp->s.next_alloc;
      } else {
        alloc_list_head = tmp->s.next_alloc;
      }
      break;
    }
    prev = tmp;
    tmp = tmp->s.next_alloc;
  }

  header->s.is_free = 1;

  tmp = header->s.next;
  while (tmp && tmp->s.is_free) {
    header_t *next_block = tmp->s.next;
    header->s.size += sizeof(header_t) + tmp->s.size;
    header->s.next = next_block;

    if (tmp == tail) {
      tail = header;
    }
    tmp = next_block;
  }

  pthread_mutex_unlock(&global_malloc_lock);
}

/*
 * @brief: Frees all remaining allocated memory blocks at program exit
 */
static void cleanup_a(void) {
  pthread_mutex_lock(&global_malloc_lock);

  header_t *curr = alloc_list_head;

  while (curr) {
    header_t *next = curr->s.next_alloc;
    if (!curr->s.is_free) {
      pthread_mutex_unlock(&global_malloc_lock);
      free_a((void *)(curr + 1));
      pthread_mutex_lock(&global_malloc_lock);
    }
    curr = next;
  }

  pthread_mutex_unlock(&global_malloc_lock);
}

/*
 * Manual declaration of the standard library atexit() function to avoid
 * including <stdlib.h>. The extern "C" guards ensure proper C linkage
 * when this header is included from C++ source files.
 */
#ifdef __cplusplus
extern "C" {
#endif

extern int atexit(void (*func)(void));

#ifdef __cplusplus
}
#endif

/*
 * @brief Initializer function that registers cleanup at program start
 */
__attribute__((constructor)) void init_a(void) { atexit(cleanup_a); }

void *calloc_a(size_t num, size_t nsize) {
  size_t size;
  void *block;

  if (!num || !nsize)
    return NULL;

  size = num * nsize;
  if (nsize != size / num)
    return NULL;

  block = malloc_a(size);
  if (!block)
    return NULL;

  memset(block, 0, size);

  return block;
}

void *realloc_a(void *block, size_t size) {
  header_t *header;
  void *ret;

  if (!block)
    return malloc_a(size);
  if (!size) {
    free_a(block);
    return NULL;
  }

  header = (header_t *)block - 1;
  if (header->s.size >= size)
    return block;

  ret = malloc_a(size);
  if (ret) {
    memcpy(ret, block, header->s.size);
    free_a(block);
  }

  return ret;
}
