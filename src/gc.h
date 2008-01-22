#ifndef JZ_GC_H
#define JZ_GC_H

/* TODO: GET RID OF THIS */
#include <stdio.h>

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

#define JZ_GC_MARK_VAL_GRAY(jz, val) {          \
    if (JZ_TVAL_CAN_BE_GCED(val)) {             \
      jz_gc_header* obj = (val).value.gc;       \
      jz_gc_mark_gray((jz), obj);               \
    }                                           \
  }

#define JZ_GC_WRITE_BARRIER_ACTIVE(jz) ((jz)->gc.state == jz_gcs_marking)

jz_gc_header* jz_gc_malloc(JZ_STATE, jz_type type, size_t size);
jz_gc_header* jz_gc_dyn_malloc(JZ_STATE, jz_type type, size_t struct_size,
                            size_t extra_size, size_t number);

void jz_gc_tick(JZ_STATE);

bool jz_gc_mark_gray(JZ_STATE, jz_gc_header* obj);

#endif
