#include "allocator.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct Node {
  int value;
  struct Node *left;
  struct Node *right;
  char padding[32];
} Node;

Node *allocate_tree(int depth, int start_value) {
  if (depth <= 0)
    return NULL;

  Node *node = malloc_a(sizeof(Node));
  if (!node) {
    printf("Allocation failed at depth %d\n", depth);
    return NULL;
  }

  node->value = start_value;
  memset(node->padding, 0xAB, sizeof(node->padding));

  node->left = allocate_tree(depth - 1, start_value * 2);
  node->right = allocate_tree(depth - 1, start_value * 2 + 1);

  return node;
}

void test_binary_tree(int depth) {
  printf("Test: Allocating binary tree of depth %d\n", depth);

  Node *root = allocate_tree(depth, 1);
  if (!root) {
    printf("Allocation failed\n");
    return;
  }

  printf("Root value: %d\n", root->value);

  printf("binary tree test complete\n");
}

void test_calloc_a_array(int count, int element_size) {
  printf("Test: Allocating array with calloc_a (count=%d, element_size=%d)\n",
         count, element_size);
  void *arr = calloc_a(count, element_size);
  if (!arr) {
    printf("Calloc allocation failed\n\n");
    return;
  }

  unsigned char *data = arr;
  printf("Element 0: %d, Element %d: %d\n", data[0], count - 1,
         data[(count - 1) * element_size]);

  printf("calloc_a test complete\n");
}

void test_realloc_a() {
  printf("Test: realloc_a scenarios\n");

  int initial_size = 10;
  int *arr = malloc_a(initial_size * sizeof(int));
  if (!arr) {
    printf("Initial malloc_a failed\n\n");
    return;
  }

  for (int i = 0; i < initial_size; ++i)
    arr[i] = i * 10;

  int new_size = 20;
  int *larger_arr = realloc_a(arr, new_size * sizeof(int));
  if (!larger_arr) {
    printf("realloc_a to larger size failed\n");
    return;
  }

  for (int i = initial_size; i < new_size; ++i)
    larger_arr[i] = i * 10;

  printf("Larger array contents:\n");
  for (int i = 0; i < new_size; ++i)
    printf("%d ", larger_arr[i]);
  printf("\n");

  int *new_arr = realloc_a(NULL, 5 * sizeof(int));
  if (!new_arr) {
    printf("realloc_a(NULL) failed\n");
  } else {
    for (int i = 0; i < 5; ++i)
      new_arr[i] = i + 100;
    printf("realloc_a(NULL) allocated array:\n");
    for (int i = 0; i < 5; ++i)
      printf("%d ", new_arr[i]);
    printf("\n");
  }

  printf("realloc_a test complete\n");
}

#define NUM_THREADS 4
#define TREE_DEPTH 3

void *thread_func(void *arg) {
  int thread_id = *(int *)arg;
  printf("Thread %d: starting allocation\n", thread_id);

  Node *root = allocate_tree(TREE_DEPTH, thread_id * 1000);
  if (!root) {
    printf("Thread %d: allocation failed\n", thread_id);
    return NULL;
  }

  printf("Thread %d: tree root value = %d\n", thread_id, root->value);

  return NULL;
}

void test_multithreaded(void) {
  pthread_t threads[NUM_THREADS];
  int thread_ids[NUM_THREADS];

  printf("Starting multithreaded allocator test with %d threads\n",
         NUM_THREADS);

  for (int i = 0; i < NUM_THREADS; ++i) {
    thread_ids[i] = i + 1;
    if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
      printf("Failed to create thread %d\n", i + 1);
    }
  }

  for (int i = 0; i < NUM_THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }

  printf("multithreaded test complete\n");
}

#define FREE_TEST_THREADS 8
#define ALLOCS_PER_THREAD 50

void *free_test_thread_func(void *arg) {
  int thread_id = *(int *)arg;
  void *allocations[ALLOCS_PER_THREAD];

  for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
    size_t size = (i + 1) * 16; // Varying sizes
    allocations[i] = malloc_a(size);
    if (!allocations[i]) {
      printf("Thread %d: allocation %d failed\n", thread_id, i);
      for (int j = 0; j < i; j++) {
        free_a(allocations[j]);
      }
      return NULL;
    }
    memset(allocations[i], thread_id, size);
  }

  printf("Thread %d: allocated %d blocks\n", thread_id, ALLOCS_PER_THREAD);

  for (int i = 0; i < ALLOCS_PER_THREAD; i += 2) {
    free_a(allocations[i]);
  }

  for (int i = 1; i < ALLOCS_PER_THREAD; i += 2) {
    free_a(allocations[i]);
  }

  printf("Thread %d: freed all %d blocks\n", thread_id, ALLOCS_PER_THREAD);

  return NULL;
}

void test_multithreaded_free(void) {
  pthread_t threads[FREE_TEST_THREADS];
  int thread_ids[FREE_TEST_THREADS];

  printf("Starting multithreaded free test with %d threads (%d allocs each)\n",
         FREE_TEST_THREADS, ALLOCS_PER_THREAD);

  for (int i = 0; i < FREE_TEST_THREADS; ++i) {
    thread_ids[i] = i + 1;
    if (pthread_create(&threads[i], NULL, free_test_thread_func,
                       &thread_ids[i]) != 0) {
      printf("Failed to create thread %d\n", i + 1);
    }
  }

  for (int i = 0; i < FREE_TEST_THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }

  printf("multithreaded free test complete\n");
}

#define elapsed_ms(start, end)                                                 \
  (end.tv_sec - start.tv_sec) * 1000.0 +                                       \
      (end.tv_nsec - start.tv_nsec) / 1000000.0

int main() {
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);
  test_binary_tree(4);
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("test_binary_tree(4) took %.3f ms\n\n", elapsed_ms(start, end));

  clock_gettime(CLOCK_MONOTONIC, &start);
  test_calloc_a_array(100, sizeof(int));
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("test_calloc_a_array took %.3f ms\n\n", elapsed_ms(start, end));

  clock_gettime(CLOCK_MONOTONIC, &start);
  test_realloc_a();
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("test_realloc_a took %.3f ms\n\n", elapsed_ms(start, end));

  clock_gettime(CLOCK_MONOTONIC, &start);
  test_multithreaded();
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("test_multithreaded took %.3f ms\n\n", elapsed_ms(start, end));

  clock_gettime(CLOCK_MONOTONIC, &start);
  test_multithreaded_free();
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("test_multithreaded_free took %.3f ms\n\n", elapsed_ms(start, end));

  return 0;
}
