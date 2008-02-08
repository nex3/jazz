#include "jazz.h"
#include "object.h"

#define jz_proto_new(jz, name) jz_proto_new1(jz, jz_str_from_literal(jz, name))
jz_obj* jz_proto_new1(JZ_STATE, jz_str* name);

#define jz_get_proto(jz, name) jz_get_proto1(jz, jz_str_from_literal(jz, name))
jz_obj* jz_get_proto1(JZ_STATE, jz_str* name);

#define jz_inst(jz, name) jz_inst1(jz, jz_str_from_literal(jz, name))
jz_obj* jz_inst1(JZ_STATE, jz_str* name);
