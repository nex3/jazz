#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "gc.h"
#include "state.h"
#include "string.h"

#define MARK_BLACK(obj) \
  (JZ_GC_TAG(obj) = (jz)->gc.black_bit | (JZ_GC_TAG(obj) & 0xfc))
#define MARK_WHITE(obj) \
  (JZ_GC_TAG(obj) = !(jz)->gc.black_bit | (JZ_GC_TAG(obj) & 0xfc))

static void blacken(JZ_STATE, jz_gc_header* obj);
static void blacken_str(JZ_STATE, jz_str* str);
#define blacken_str_value(jz, val) /* String values have no references. */

static jz_gc_header* pop_gray_stack(JZ_STATE);
static void mark_roots(JZ_STATE);

jz_gc_header* jz_gc_malloc(JZ_STATE, jz_type type, size_t size) {
  jz_gc_header* to_ret;

  assert(size >= sizeof(jz_gc_header));
  to_ret = calloc(size, 1); /* calloc zeroes memory. */
  JZ_GC_SET_TYPE(to_ret, type);
  JZ_GC_SET_UTAG(to_ret, 0);
  MARK_WHITE(to_ret);

  to_ret->next = jz->gc.all_objs;
  jz->gc.all_objs = to_ret;

  return to_ret;
}

jz_gc_header* jz_gc_dyn_malloc(JZ_STATE, jz_type type, size_t struct_size,
                            size_t extra_size, size_t number) {
  assert(struct_size - extra_size >= sizeof(jz_gc_header));
  /* calloc zeroes memory. */
  return jz_gc_malloc(jz, type, struct_size + extra_size * (number - 1));
}

bool jz_gc_mark_gray(JZ_STATE, jz_gc_header* obj) {
  char tag = JZ_GC_TAG(obj);

  if (tag & 0x02 || (tag & 0x01) == jz->gc.black_bit) return false;
  else {
    jz_gc_node* node = malloc(sizeof(jz_gc_node));

    node->next = jz->gc.gray_stack;
    node->obj = obj;
    jz->gc.gray_stack = node;

    JZ_GC_TAG(obj) |= 0x02;
    return true;
  }
}

void jz_gc_tick(JZ_STATE) {
  switch (jz->gc.state) {
  case jz_gcs_waiting:
    mark_roots(jz);
    jz->gc.state = jz_gcs_marking;
    return;

  case jz_gcs_marking: {
    jz_gc_header* obj = pop_gray_stack(jz);
    if (obj == NULL) jz->gc.state = jz_gcs_sweeping;
    else blacken(jz, obj);
    return;
  }

  case jz_gcs_sweeping:
    break;

  default:
    fprintf(stderr, "Unknown garbage-collection state %d\n", jz->gc.state);
    exit(1);
  }
}

void blacken(JZ_STATE, jz_gc_header* obj) {
  switch (JZ_GC_TYPE(obj)) {
  case jz_strt:
    blacken_str(jz, (jz_str*)obj);
    break;
  case jz_str_valuet:
    blacken_str_value(jz, (jz_str_value*)obj);
    break;
  default:
    fprintf(stderr, "Unknown GC type %d\n", JZ_GC_TYPE(obj));
    exit(1);
  }
  MARK_BLACK(obj);
}

void blacken_str(JZ_STATE, jz_str* str) {
  if (JZ_STR_IS_INT(str)) jz_gc_mark_gray(jz, &str->value.val->gc);
}

jz_gc_header* pop_gray_stack(JZ_STATE) {
  jz_gc_node* node = jz->gc.gray_stack;
  jz_gc_header* obj;

  if (node == NULL) return NULL;

  obj = node->obj;
  jz->gc.gray_stack = node->next;
  free(node);

  return obj;
}

void mark_roots(JZ_STATE) {
  jz_frame* frame = jz->current_frame;
  jz_tvalue* next = frame->locals_then_stack;
  jz_tvalue* top = *frame->stack_top;

  for (; next != top; next++) JZ_GC_MARK_VAL_GRAY(jz, *next);

  next = frame->bytecode->consts;
  top = next + frame->bytecode->consts_length;
  for (; next != top; next++) JZ_GC_MARK_VAL_GRAY(jz, *next);
}
