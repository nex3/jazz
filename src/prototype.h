#ifndef JZ_PROTO_H
#define JZ_PROTO_H

#include "jazz.h"
#include "object.h"

typedef void jz_proto_callback(JZ_STATE, jz_obj* obj);

struct jz_proto {
  jz_gc_header gc;
  jz_obj* obj;
  jz_str* class;

/* Note: any Jazz objects referenced by obj
   may be garbage collected before the finalizer is run.
   Thus, finalizers shouldn't attempt to dereference Jazz objects
   that aren't referenced elsewhere.

   Also note that the default finalizer
   just frees obj->data if it exists.

   TODO: Might want to find some way to fix this.
   Other languages' solutions seem to be very complicated. */
  jz_proto_callback* finalizer;
  jz_proto_callback* marker;
};

#define jz_proto_new(jz, name) jz_proto_new1(jz, jz_str_from_literal(jz, name))
jz_proto* jz_proto_new1(JZ_STATE, jz_str* name);

#define jz_get_proto(jz, name) jz_get_proto1(jz, jz_str_from_literal(jz, name))
jz_proto* jz_get_proto1(JZ_STATE, jz_str* name);

#define jz_inst(jz, name) jz_inst1(jz, jz_str_from_literal(jz, name))
jz_obj* jz_inst1(JZ_STATE, jz_str* name);

#endif
