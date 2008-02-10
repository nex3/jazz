#include <stdio.h>

#include "prototype.h"
#include "state.h"

jz_proto* jz_proto_new1(JZ_STATE, jz_str* name) {
  jz_proto* proto = (jz_proto*)jz_gc_malloc(jz, jz_t_proto, sizeof(jz_proto));
  jz_str* obj_str = jz_str_from_literal(jz, "Object");

  proto->obj = jz_obj_new_bare(jz);

  /* All prototypes other than Object have the Object prototype
     as their own prototype. */
  if (!jz_str_equal(jz, name, obj_str))
    proto->obj->prototype = jz_get_proto1(jz, obj_str);

  proto->class = name;
  jz_obj_put(jz, jz->prototypes, name, jz_wrap_proto(jz, proto));

  return proto;
}

jz_proto* jz_get_proto1(JZ_STATE, jz_str* name) {
  jz_tvalue obj = jz_obj_get(jz, jz->prototypes, name);

  if (JZ_TVAL_TYPE(obj) != jz_t_proto)
    return NULL;

  return obj.value.proto;
}

jz_obj* jz_inst1(JZ_STATE, jz_str* name) {
  jz_proto* proto = jz_get_proto1(jz, name);
  jz_obj* obj = jz_obj_new_bare(jz);

  if (proto == NULL) {
    fprintf(stderr, "No prototype %s exists\n", jz_str_to_chars(jz, name));
    exit(1);
  }

  obj->prototype = proto;

  return obj;
}
