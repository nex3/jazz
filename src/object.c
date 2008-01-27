#include "object.h"

jz_obj* jz_obj_new(JZ_STATE) {
  return (jz_obj*)jz_gc_malloc(jz, jz_t_obj, sizeof(jz_obj));
}
