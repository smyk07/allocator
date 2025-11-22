#include "allocator.h"

#include <stdio.h>

int main() {
  int *a = malloc_a(sizeof(int));

  *a = 2;

  printf("%d\n", *a);

  free_a(a);
}
