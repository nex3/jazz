#include <stdio.h>
#include <assert.h>

#include "function.h"
#include "object.h"
#include "prototype.h"
#include "state.h"

#define ARG(i) ((i) < argc ? argv[i] : JZ_UNDEFINED)

typedef struct func_data {
  int arity;
} func_data;

jz_tvalue jz_call_arr(JZ_STATE, jz_obj* func, int argc, const jz_tvalue* argv) {
  jz_args* args;
  jz_tvalue ret;

  if (func->call == NULL) {
    fprintf(stderr, "TypeError: %s is not a function.\n", jz_str_to_chars(jz, jz_to_str(jz, jz_wrap_obj(jz, func))));
    exit(1);
  }

  args = malloc(sizeof(jz_args));
  args->callee = func;
  args->length = argc;
  args->argv = argv;

  switch (((func_data*)func->data)->arity) {
  case JZ_ARITY_VAR:
    ret = func->call(jz, args, argc, argv);
    break;
  case 0:
    ret = func->call(jz, args);
    break;

    /* This is hideous, but I don't think there's a better way to do it. */
  case 1:
    ret = func->call(jz, args, ARG(0));
    break;
  case 2:
    ret = func->call(jz, args, ARG(0), ARG(1));
    break;
  case 3:
    ret = func->call(jz, args, ARG(0), ARG(1), ARG(2));
    break;
  case 4:
    ret = func->call(jz, args, ARG(0), ARG(1), ARG(2),
                     ARG(4));
    break;
  case 5:
    ret = func->call(jz, args, ARG(0), ARG(1), ARG(2),
                     ARG(4), ARG(5));
    break;
  case 6:
    ret = func->call(jz, args, ARG(0), ARG(1), ARG(2),
                     ARG(4), ARG(5), ARG(6));
    break;
  case 7:
    ret = func->call(jz, args, ARG(0), ARG(1), ARG(2),
                     ARG(4), ARG(5), ARG(6), ARG(7));
    break;
  case 8:
    ret = func->call(jz, args, ARG(0), ARG(1), ARG(2),
                     ARG(4), ARG(5), ARG(6), ARG(7), ARG(8));
    break;
  }

  free(args);
  return ret;
}

jz_obj* jz_fn_to_obj(JZ_STATE, jz_fn* fn, int arity) {
  jz_obj* obj = jz_inst(jz, "Function");
  jz_obj* proto = jz_obj_new(jz);
  func_data* data = malloc(sizeof(func_data));

  obj->call = fn;

  data->arity = arity;
  obj->data = data;
  jz_obj_put2(jz, obj, "length", jz_wrap_num(jz, arity));

  jz_obj_put2(jz, proto, "constructor", jz_wrap_obj(jz, obj));
  jz_obj_put2(jz, obj, "prototype", jz_wrap_obj(jz, proto));

  return obj;
}

void jz_init_func_proto(JZ_STATE) {
  jz_proto_new(jz, "Function");
}
