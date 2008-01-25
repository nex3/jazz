#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "gc.h"
#include "state.h"
#include "string.h"

#define MARK_BLACK(obj) \
  (JZ_GC_TAG(obj) = jz->gc.black_bit | (JZ_GC_TAG(obj) & 0xfc))
#define MARK_WHITE(obj) \
  (JZ_GC_TAG(obj) = !jz->gc.black_bit | (JZ_GC_TAG(obj) & 0xfc))

static void blacken(JZ_STATE, jz_gc_header* obj);
static void blacken_str(JZ_STATE, jz_str* str);
#define blacken_str_value(jz, val) /* String values have no references. */

static jz_gc_header* pop_gray_stack(JZ_STATE);
static void mark_roots(JZ_STATE);
static void mark_step(JZ_STATE);
static bool sweep_step(JZ_STATE);

static void finish_cycle(JZ_STATE);

jz_gc_header* jz_gc_malloc(JZ_STATE, jz_type type, size_t size) {
  jz_gc_header* to_ret;

  assert(size >= sizeof(jz_gc_header));
  to_ret = calloc(size, 1); /* calloc zeroes memory. */
  JZ_GC_SET_TYPE(to_ret, type);
  JZ_GC_SET_UTAG(to_ret, 0);
  MARK_WHITE(to_ret);

  to_ret->next = jz->gc.all_objs;
  jz->gc.all_objs = to_ret;

  jz->gc.allocated += size;

  return to_ret;
}

jz_gc_header* jz_gc_dyn_malloc(JZ_STATE, jz_type type, size_t struct_size,
                            size_t extra_size, size_t number) {
  assert(struct_size - extra_size >= sizeof(jz_gc_header));
  return jz_gc_malloc(jz, type, struct_size + extra_size * (number - 1));
}

bool jz_gc_mark_gray(JZ_STATE, jz_gc_header* obj) {
  jz_gc_node* node;

  /* If the object is already black,
     we don't need to do anything about it. */
  if (jz_gc_is_black(jz, obj))
    return false;

  node = malloc(sizeof(jz_gc_node));

  node->next = jz->gc.gray_stack;
  node->obj = obj;
  jz->gc.gray_stack = node;
  MARK_BLACK(obj);

  return true;
}

void jz_gc_cycle(JZ_STATE) {
  /* Make sure we finish up an existing cycle
     before we start a new one. */
  if (!jz_gc_paused(jz))
    while (!jz_gc_step(jz));
  while (!jz_gc_step(jz));
}

bool jz_gc_steps(JZ_STATE) {
  unsigned char i;
  unsigned char steps = jz->gc.speed;

  for (i = 0; i < steps; i++) {
    if (jz_gc_step(jz))
      /* We don't want to run over into a new collection cycle. */
      return true;
  }
  return false;
}

bool jz_gc_step(JZ_STATE) {
  switch (jz->gc.state) {
  case jz_gcs_waiting:
    mark_roots(jz);
    jz->gc.state = jz_gcs_marking;
    return false;

  case jz_gcs_marking:
    mark_step(jz);
    return false;

  case jz_gcs_sweeping:
    return sweep_step(jz);

  default:
    fprintf(stderr, "Unknown garbage-collection state %d\n", jz->gc.state);
    exit(1);
  }
}

void blacken(JZ_STATE, jz_gc_header* obj) {
  switch (JZ_GC_TYPE(obj)) {
  case jz_t_str:
    blacken_str(jz, (jz_str*)obj);
    break;
  case jz_t_str_value:
    blacken_str_value(jz, (jz_str_value*)obj);
    break;
  default:
    fprintf(stderr, "Unknown GC type %d\n", JZ_GC_TYPE(obj));
    exit(1);
  }
  /* We don't actually need to blacken the object in this method,
     because it was blackened when it was added to the gray stack. */
}

void blacken_str(JZ_STATE, jz_str* str) {
  if (JZ_STR_IS_INT(str) && str->value.val != NULL)
    jz_gc_mark_gray(jz, &str->value.val->gc);
}

jz_gc_header* pop_gray_stack(JZ_STATE) {
  jz_gc_node* node = jz->gc.gray_stack;
  jz_gc_header* obj;

  if (node == NULL)
    return NULL;

  obj = node->obj;
  jz->gc.gray_stack = node->next;
  free(node);

  return obj;
}

void mark_roots(JZ_STATE) {
  jz_frame* frame = jz->current_frame;
  jz_tvalue* next;
  jz_tvalue* top;

  if (frame == NULL)
    return;

  next = frame->locals_then_stack;
  top = *frame->stack_top;

  for (; next != top; next++)
    JZ_GC_MARK_VAL_GRAY(jz, *next);

  next = frame->bytecode->consts;
  top = next + frame->bytecode->consts_length;
  for (; next != top; next++)
    JZ_GC_MARK_VAL_GRAY(jz, *next);
}

void mark_step(JZ_STATE) {
  jz_gc_header* obj = pop_gray_stack(jz);
  if (obj == NULL) {
    jz->gc.black_bit = !jz->gc.black_bit;
    jz->gc.state = jz_gcs_sweeping;
  }
  else blacken(jz, obj);
  return;
}

bool sweep_step(JZ_STATE) {
  jz_gc_header* prev = jz->gc.prev_sweep_obj;
  jz_gc_header* next = jz->gc.next_sweep_obj;

  /* By the time we reach this function,
     the white and black bits have been flipped.
     Thus, black objects are colletable and white objects are not. */

  if (next == NULL) {
    next = jz->gc.all_objs;

    if (next == NULL) {
      /* For some reason, there are no heap-allocated objects. */
      finish_cycle(jz);
      return true;
    }

    if (jz_gc_is_black(jz, next)) {
      jz->gc.all_objs = next->next;
      free(next);
      return false;
    }
  }

  for (; next != NULL; prev = next, next = next->next) {
    if (jz_gc_is_white(jz, next))
      continue;

    prev->next = next->next;
    free(next);

    jz->gc.prev_sweep_obj = prev;
    jz->gc.next_sweep_obj = prev->next;

    return false;
  }

  /* No more black (sweepable) objects left. */
  finish_cycle(jz);

  return true;
}

void finish_cycle(JZ_STATE) {
  jz->gc.prev_sweep_obj = NULL;
  jz->gc.next_sweep_obj = NULL;
  jz->gc.state = jz_gcs_waiting;
  jz->gc.threshold = (jz->gc.pause * jz->gc.allocated)/100;
  assert(jz->gc.threshold > 0);
}
