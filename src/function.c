#include <stdio.h>
#include <assert.h>

#include "function.h"
#include "object.h"
#include "prototype.h"
#include "state.h"
#include "vm.h"

#define ARG(i) ((i) < argc ? argv[i] : JZ_UNDEFINED)

static jz_obj* create_func(JZ_STATE, int arity);
static jz_val call_jazz_func(JZ_STATE, jz_args* args, int argc, const jz_val* argv);
static void finalizer(JZ_STATE, jz_obj* obj);
static void marker(JZ_STATE, jz_obj* obj);

jz_val jz_call_arr(JZ_STATE, jz_obj* func, int argc, const jz_val* argv) {
  jz_args* args;
  jz_val ret;

  if (func->call == NULL) {
    fprintf(stderr, "TypeError: %s is not a function.\n", jz_str_to_chars(jz, jz_to_str(jz, func)));
    exit(1);
  }

  args = malloc(sizeof(jz_args));
  args->callee = func;
  args->length = argc;
  args->argv = argv;

  switch (((jz_func_data*)func->data)->arity) {
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

jz_obj* create_func(JZ_STATE, int arity) {
  jz_obj* obj = jz_inst(jz, "Function");
  jz_obj* proto = jz_obj_new(jz);

  /* jz_func_data includes room for one byte of arity calculation.
     This cancels out the rounding-down of integer division. */
  jz_func_data* data = malloc(sizeof(jz_func_data));

  obj->data = data;
  data->code = NULL;
  data->arity = 0;
  data->scope = NULL;
  data->closure_vars = NULL;

  jz_obj_put2(jz, obj, "length", jz_wrap_num(jz, arity));
  jz_obj_put2(jz, proto, "constructor", obj);
  jz_obj_put2(jz, obj, "prototype", proto);

  return obj;
}

jz_obj* jz_func_new(JZ_STATE, jz_bytecode* code, int arity) {
  jz_obj* obj = create_func(jz, arity);
  jz_func_data* data = JZ_FUNC_DATA(obj);

  obj->call = call_jazz_func;
  data->arity = JZ_ARITY_VAR;
  data->code = code;

  return obj;
}

void jz_func_set_scope(JZ_STATE, jz_obj* this, jz_frame* scope) {
  jz_func_data* data = JZ_FUNC_DATA(this);

  data->scope = scope->closure_locals;
  data->closure_vars = calloc(sizeof(jz_val*), scope->bytecode->closure_vars_length);
  memcpy(data->closure_vars, JZ_FRAME_CLOSURE_VARS(scope),
         scope->bytecode->closure_vars_length * sizeof(jz_val*));
}

jz_val call_jazz_func(JZ_STATE, jz_args* args, int argc, const jz_val* argv) {
  jz_frame* frame = jz_frame_new_from_func(jz, args->callee);
  jz_byte* param_locs = JZ_FUNC_DATA(args->callee)->code->param_locs;
  jz_val** closure_vars = JZ_FRAME_CLOSURE_VARS(frame);
  jz_val* locals = JZ_FRAME_LOCALS(frame);
  int i = 0;

  for (; i < argc; i++, argv++) {
    if (JZ_BITFIELD_GET(param_locs, i)) {
      **closure_vars = *argv;
      closure_vars++;
    } else {
      *locals = *argv;
      locals++;
    }
  }

  return jz_vm_run_frame(jz, frame);
}

jz_obj* jz_fn_to_obj(JZ_STATE, jz_fn* fn, int arity) {
  jz_obj* obj = create_func(jz, arity);
  jz_func_data* data = JZ_FUNC_DATA(obj);

  obj->call = fn;
  data->arity = arity;

  return obj;
}

void jz_init_func_proto(JZ_STATE) {
  jz_proto* proto = jz_proto_new(jz, "Function");

  proto->finalizer = finalizer;
  proto->marker = marker;
}

void finalizer(JZ_STATE, jz_obj* obj) {
  jz_func_data* data = (jz_func_data*)obj->data;
  jz_free_bytecode(jz, data->code);
  free(data->closure_vars);
  free(data);
}

void marker(JZ_STATE, jz_obj* obj) {
  jz_func_data* data = (jz_func_data*)obj->data;

  if (data->scope)
    jz_gc_mark_gray(jz, &data->scope->gc);

  if (data->code)
    jz_mark_bytecode(jz, data->code);
}
