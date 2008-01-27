#include "object.h"
#include "string.h"

jz_obj* jz_obj_new(JZ_STATE) {
  return (jz_obj*)jz_gc_malloc(jz, jz_t_obj, sizeof(jz_obj));
}

jz_tvalue jz_obj_to_str(JZ_STATE, jz_obj* obj) {
  /* TODO: Replace calls to this with actual calls to toString(). */
  return jz_wrap_str(jz, jz_str_from_literal(jz, "[object Object]"));
}

jz_tvalue jz_obj_value_of(JZ_STATE, jz_obj* obj) {
  /* TODO: Replace calls to this with actual calls to toString(). */
  return jz_wrap_obj(jz, obj);
}
