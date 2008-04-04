#ifndef JZ_GC_H
#define JZ_GC_H

#include <stdlib.h>

#include "jazz.h"
#include "value.h"

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

#define JZ_GC_TYPE(obj) (JZ_TAG_TYPE(JZ_GC_TAG(obj)))
#define JZ_GC_SET_TYPE(obj, type) \
  (JZ_GC_TAG(obj) = JZ_TAG_WITH_TYPE(JZ_GC_TAG(obj), type))

#define JZ_GC_FLAG_BIT 0
#define JZ_GC_FLAG(obj) (JZ_BIT(JZ_GC_TAG(obj), JZ_GC_FLAG_BIT))

#define jz_gc_is_white(jz, obj)                 \
  (JZ_GC_FLAG(obj) == !jz->gc.black_bit)
#define jz_gc_is_black(jz, obj)                 \
  (JZ_GC_FLAG(obj) == jz->gc.black_bit)

#define JZ_GC_MARK_VAL_GRAY(jz, val)                                    \
  (JZ_VAL_CAN_BE_GCED(val) ?                                            \
   jz_gc_mark_gray((jz), (jz_gc_header*)val) :                          \
   jz_false)

#define jz_gc_write_barrier_active(jz) (jz->gc.state == jz_gcs_marking)
#define jz_gc_paused(jz) (jz->gc.state == jz_gcs_waiting)
#define jz_gc_within_threshold(jz) (jz->gc.allocated < jz->gc.threshold)

#define JZ_GC_WRITE_BARRIER(jz, referrer, reference)            \
  ((jz_gc_write_barrier_active(jz) &&                           \
    jz_gc_is_black(jz, (jz_gc_header*)(referrer)) &&            \
    jz_gc_is_white(jz, (jz_gc_header*)(reference))) ?           \
   jz_gc_mark_gray(jz, (jz_gc_header*)(reference)) : jz_false)

#define JZ_GC_WRITE_BARRIER_VAL(jz, referrer, val) {    \
    if (JZ_VAL_CAN_BE_GCED(val)) {                      \
      jz_gc_header* obj = (jz_gc_header*)val;           \
      JZ_GC_WRITE_BARRIER((jz), referrer, obj);         \
    }                                                   \
  }

jz_gc_header* jz_gc_malloc(JZ_STATE, jz_type type, size_t size);
jz_gc_header* jz_gc_dyn_malloc(JZ_STATE, jz_type type, size_t struct_size,
                            size_t extra_size, size_t number);

#define jz_gc_tick(jz)                                          \
  ((jz_gc_within_threshold(jz) && jz_gc_paused(jz)) ? jz_false :   \
   jz_gc_steps(jz))

void jz_gc_cycle(JZ_STATE);
jz_bool jz_gc_steps(JZ_STATE);
jz_bool jz_gc_step(JZ_STATE);

#define jz_gc_set_speed(jz, new_speed) ((jz)->gc.speed = (new_speed))
#define jz_gc_set_pause(jz, new_pause) ((jz)->gc.pause = (new_pause))

jz_bool jz_gc_mark_gray(JZ_STATE, jz_gc_header* obj);
void jz_mark_frame(JZ_STATE, jz_frame* frame);

void jz_gc_init(JZ_STATE);

#endif
