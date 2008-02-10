#ifndef JZ_PROTO_H
#define JZ_PROTO_H

#include "jazz.h"
#include "object.h"

struct jz_proto {
  jz_gc_header gc;
  jz_obj* obj;
  jz_str* class;
};

#define jz_proto_new(jz, name) jz_proto_new1(jz, jz_str_from_literal(jz, name))
jz_proto* jz_proto_new1(JZ_STATE, jz_str* name);

#define jz_get_proto(jz, name) jz_get_proto1(jz, jz_str_from_literal(jz, name))
jz_proto* jz_get_proto1(JZ_STATE, jz_str* name);

#define jz_inst(jz, name) jz_inst1(jz, jz_str_from_literal(jz, name))
jz_obj* jz_inst1(JZ_STATE, jz_str* name);

#endif
