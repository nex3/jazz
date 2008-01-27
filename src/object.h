#ifndef JZ_OBJECT_H
#define JZ_OBJECT_H

#include "jazz.h"
#include "gc.h"

struct jz_obj {
  jz_gc_header gc;
};

jz_obj* jz_obj_new(JZ_STATE);

#define jz_obj_null(jz) ((jz_obj*)NULL)
#define jz_obj_is_null(jz, obj) ((obj) == NULL)

#endif
