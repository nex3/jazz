#include "core/global.h"
#include "state.h"
#include "function.h"
#include "object.h"

#include <stdio.h>

static jz_tvalue is_nan(JZ_STATE, jz_args* args, jz_tvalue arg) {
  double num = jz_to_num(jz, arg);
  return jz_wrap_bool(jz, JZ_NUM_IS_NAN(num));
}

void jz_init_global(JZ_STATE) {
  jz_obj* obj = jz->global_obj;

  jz_obj_put2(jz, obj, "isNaN", jz_wrap_fn(jz, is_nan, 1));
}
