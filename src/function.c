#include <stdio.h>
#include <assert.h>

#include "function.h"
#include "object.h"
#include "prototype.h"
#include "state.h"
#include "vm.h"

#define ARG(i) ((i) < argc ? argv[i] : JZ_UNDEFINED)

typedef struct func_data {
  int arity;
  jz_bytecode* code;
} func_data;

static jz_obj* create_func(JZ_STATE, int arity, func_data** data);
static jz_tvalue call_jazz_func(JZ_STATE, jz_args* args, int argc, const jz_tvalue* argv);
static void finalizer(JZ_STATE, jz_obj* obj);

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

jz_obj* create_func(JZ_STATE, int arity, func_data** data) {
  jz_obj* obj = jz_inst(jz, "Function");
  jz_obj* proto = jz_obj_new(jz);

  *data = malloc(sizeof(func_data));
  (*data)->code = NULL;
  (*data)->arity = 0;
  obj->data = *data;

  jz_obj_put2(jz, obj, "length", jz_wrap_num(jz, arity));
  jz_obj_put2(jz, proto, "constructor", jz_wrap_obj(jz, obj));
  jz_obj_put2(jz, obj, "prototype", jz_wrap_obj(jz, proto));

  return obj;
}

jz_obj* jz_func_new(JZ_STATE, jz_bytecode* code, int arity) {
  func_data* data;
  jz_obj* obj = create_func(jz, arity, &data);

  obj->call = call_jazz_func;
  data->arity = JZ_ARITY_VAR;
  data->code = code;

  return obj;
}

jz_tvalue call_jazz_func(JZ_STATE, jz_args* args, int argc, const jz_tvalue* argv) {
  return jz_vm_run(jz, ((func_data*)args->callee->data)->code);
}

jz_obj* jz_fn_to_obj(JZ_STATE, jz_fn* fn, int arity) {
  func_data* data;
  jz_obj* obj = create_func(jz, arity, &data);

  obj->call = fn;
  data->arity = arity;

  return obj;
}

void jz_init_func_proto(JZ_STATE) {
  jz_proto* proto = jz_proto_new(jz, "Function");

  proto->finalizer = finalizer;
}

void finalizer(JZ_STATE, jz_obj* obj) {
  func_data* data = (func_data*)obj->data;
  jz_free_bytecode(jz, data->code);
  free(data);
}
