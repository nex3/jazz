#include <stdio.h>
#include <assert.h>

#include "function.h"
#include "object.h"
#include "prototype.h"

jz_tvalue jz_call_arr(JZ_STATE, jz_obj* func, int argc, const jz_tvalue* argv) {
  jz_args* args;
  jz_tvalue ret;
  int arity;

  if (func->call == NULL) {
    fprintf(stderr, "TypeError: %s is not a function.\n", jz_str_to_chars(jz, jz_to_str(jz, jz_wrap_obj(jz, func))));
    exit(1);
  }

  args = malloc(sizeof(jz_args));
  args->callee = func;
  args->length = argc;
  args->argv = argv;

  /* TODO: Get rid of this */
  {
    jz_tvalue tval_arity = jz_obj_get2(jz, func, "length");
    double num_arity;
    assert(JZ_TVAL_TYPE(tval_arity) == jz_t_num);
    num_arity = tval_arity.value.num;
    assert(JZ_NUM_IS_INT(num_arity));
    arity = (int)num_arity;
  }

  switch (arity) {
  case JZ_ARITY_VAR:
    ret = func->call(jz, args, argc, argv);
    break;
  case 0:
    ret = func->call(jz, args);
    break;
  default:
    fprintf(stderr, "Fancy functions not yet implemented.\n");
    exit(1);
  }

  free(args);
  return ret;
}

jz_obj* jz_fn_to_obj(JZ_STATE, jz_fn* fn, int arity) {
  jz_obj* obj = jz_inst(jz, "Function");
  jz_obj* proto = jz_obj_new(jz);

  obj->call = fn;
  jz_obj_put2(jz, obj, "length", jz_wrap_num(jz, arity));

  jz_obj_put2(jz, proto, "constructor", jz_wrap_obj(jz, obj));
  jz_obj_put2(jz, obj, "prototype", jz_wrap_obj(jz, proto));

  return obj;
}

void jz_init_func_proto(JZ_STATE) {
  jz_proto_new(jz, "Function");
}
