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

typedef struct gc_header gc_header;
struct gc_header {
  gc_header* next;
  unsigned char tag;
};

#define JZ_GC_TAG(obj) (((gc_header*)obj)->tag)

#define JZ_GC_TYPE(obj) (JZ_GC_TAG(obj) >> 2)
#define JZ_GC_SET_TYPE(obj, type) \
  (JZ_GC_TAG(obj) = (((type) << 2) & 0x3f) | (JZ_GC_TAG(obj) & 0xc3))

#define JZ_GC_UTAG(obj) (JZ_GC_TAG(obj) >> 6)
#define JZ_GC_SET_UTAG(obj, val) \
  (JZ_GC_TAG(obj) = (((val) << 6) & 0xff) | (JZ_GC_TAG(obj) & 0x3f))
#define JZ_GC_UTAG_AND(obj, val) \
  (JZ_GC_TAG(obj) &= (((val) << 6) & 0xff) | 0x3f)
#define JZ_GC_UTAG_OR(obj, val) \
  (JZ_GC_TAG(obj) |= ((val) << 6) & 0xff)

gc_header* jz_gc_malloc(JZ_STATE, jz_type type, size_t size);
gc_header* jz_gc_dyn_malloc(JZ_STATE, jz_type type, size_t struct_size,
                            size_t extra_size, size_t number);

#endif
