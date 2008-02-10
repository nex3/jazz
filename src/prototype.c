#include <stdio.h>

#include "prototype.h"
#include "state.h"

jz_obj* jz_proto_new1(JZ_STATE, jz_str* name) {
  jz_obj* proto = jz_obj_new_bare(jz);
  jz_str* obj = jz_str_from_literal(jz, "Object");

  /* All prototypes other than Object have the Object prototype
     as their own prototype. */
  if (!jz_str_equal(jz, name, obj))
    proto->prototype = jz_get_proto1(jz, obj);

  proto->class = name;
  jz_obj_put(jz, jz->prototypes, name, jz_wrap_obj(jz, proto));

  return proto;
}

jz_obj* jz_get_proto1(JZ_STATE, jz_str* name) {
  jz_tvalue obj = jz_obj_get(jz, jz->prototypes, name);

  if (JZ_TVAL_TYPE(obj) != jz_t_obj)
    return NULL;

  return obj.value.obj;
}

jz_obj* jz_inst1(JZ_STATE, jz_str* name) {
  jz_obj* proto = jz_get_proto1(jz, name);
  jz_obj* obj = jz_obj_new_bare(jz);

  if (proto == NULL) {
    fprintf(stderr, "No prototype %s exists\n", jz_str_to_chars(jz, name));
    exit(1);
  }

  obj->prototype = proto;
  obj->class = proto->class;

  return obj;
}
