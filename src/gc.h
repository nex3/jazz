#ifndef JZ_GC_H
#define JZ_GC_H

#include "jazz.h"
#include "value.h"

/* The garbage collector adds an eight-bit tag
   to each of the garbage-collected structs.
   The lower two bits of this are reserved for the GC's internal use.
   The middle four bits contain a value
   of the enum jz_type (defined in value.h)
   and may be accessed with the JZ_GC_TYPE macros.
   The upper two bits may be used by individual structs
   for any tagging they need,
   and may be accessed with the JZ_GC_UTAG macros. */

struct jz_gc_header {
  jz_gc_header* next;
  unsigned char tag;
};

typedef struct jz_gc_node jz_gc_node;
struct jz_gc_node {
  jz_gc_node* next;
  jz_gc_header* obj;
};

typedef enum {
  jz_gcs_waiting,
  jz_gcs_marking,
  jz_gcs_sweeping
} jz_gc_state;

#define JZ_GC_DEFAULT_SPEED 2
#define JZ_GC_DEFAULT_PAUSE 150

#define JZ_GC_TAG(obj) (((jz_gc_header*)obj)->tag)
#define JZ_GC_NEXT(obj) (((jz_gc_header*)obj)->next)

#define JZ_GC_TYPE(obj) ((JZ_GC_TAG(obj) >> 2) & 0x0f)
#define JZ_GC_SET_TYPE(obj, type) \
  (JZ_GC_TAG(obj) = (((type) << 2) & 0x3f) | (JZ_GC_TAG(obj) & 0xc3))

#define JZ_GC_UTAG(obj) (JZ_GC_TAG(obj) >> 6)
#define JZ_GC_SET_UTAG(obj, val) \
  (JZ_GC_TAG(obj) = (((val) << 6) & 0xff) | (JZ_GC_TAG(obj) & 0x3f))
#define JZ_GC_UTAG_AND(obj, val) \
  (JZ_GC_TAG(obj) &= (((val) << 6) & 0xff) | 0x3f)
#define JZ_GC_UTAG_OR(obj, val) \
  (JZ_GC_TAG(obj) |= ((val) << 6) & 0xff)

#define JZ_GC_FLAG(obj) (JZ_GC_TAG(obj) & 0x03)

#define jz_gc_is_white(jz, obj)                 \
  (JZ_GC_FLAG(obj) == !jz->gc.black_bit)
#define jz_gc_is_black(jz, obj)                 \
  (JZ_GC_FLAG(obj) == jz->gc.black_bit)

#define JZ_GC_MARK_VAL_GRAY(jz, val) {          \
    if (JZ_TVAL_CAN_BE_GCED(val)) {             \
      jz_gc_header* obj = (val).value.gc;       \
      jz_gc_mark_gray((jz), obj);               \
    }                                           \
  }

#define jz_gc_write_barrier_active(jz) (jz->gc.state == jz_gcs_marking)
#define jz_gc_paused(jz) (jz->gc.state == jz_gcs_waiting)
#define jz_gc_within_threshold(jz) (jz->gc.allocated < jz->gc.threshold)

#define JZ_GC_WRITE_BARRIER(jz, referrer, reference)            \
  ((jz_gc_write_barrier_active(jz) &&                           \
    jz_gc_is_black(jz, (jz_gc_header*)(reference)) &&            \
    jz_gc_is_white(jz, (jz_gc_header*)(referrer))) ?           \
   jz_gc_mark_gray(jz, (jz_gc_header*)(reference)) : false)

jz_gc_header* jz_gc_malloc(JZ_STATE, jz_type type, size_t size);
jz_gc_header* jz_gc_dyn_malloc(JZ_STATE, jz_type type, size_t struct_size,
                            size_t extra_size, size_t number);

#define jz_gc_tick(jz)                                          \
  ((jz_gc_within_threshold(jz) && jz_gc_paused(jz)) ? false :   \
   jz_gc_steps(jz))

void jz_gc_cycle(JZ_STATE);
bool jz_gc_steps(JZ_STATE);
bool jz_gc_step(JZ_STATE);

#define jz_gc_set_speed(jz, new_speed) ((jz)->gc.speed = (new_speed))
#define jz_gc_set_pause(jz, new_pause) ((jz)->gc.pause = (new_pause))

bool jz_gc_mark_gray(JZ_STATE, jz_gc_header* obj);

#endif