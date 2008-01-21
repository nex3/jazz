#include <stdlib.h>
#include <assert.h>

#include "gc.h"

gc_header* jz_gc_malloc(JZ_STATE, jz_type type, size_t size) {
  gc_header* to_ret;

  assert(size >= sizeof(gc_header));
  to_ret = calloc(size, 1); /* calloc zeroes memory. */
  JZ_GC_SET_TYPE(to_ret, type);
  return to_ret;
}

gc_header* jz_gc_dyn_malloc(JZ_STATE, jz_type type, size_t struct_size,
                            size_t extra_size, size_t number) {
  gc_header* to_ret;

  assert(struct_size - extra_size >= sizeof(gc_header));
  /* calloc zeroes memory. */
  to_ret = calloc(struct_size + extra_size * (number - 1), 1);
  JZ_GC_SET_TYPE(to_ret, type);
  return to_ret;
}
