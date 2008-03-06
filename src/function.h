#ifndef JZ_FN_H
#define JZ_FN_H

#include "jazz.h"
#include "value.h"
#include "compile.h"
#include "frame.h"

/* Note that the argument list for this type is empty
   to allow arbitary arguments to be passed.

   Which arguments it accepts are determined by its arity
   (set in jz_fn_to_obj):

   JZ_ARITY_VAR:
      (JZ_STATE, jz_args* args, int argc, const jz_val* argv)
      Takes the function arguments as an array.

   0: (JZ_STATE, jz_args* args)
      Takes no arguments.

   1: (JZ_STATE, jz_args* args, jz_val arg1)
      Takes one argument.

   2: (JZ_STATE, jz_args* args, jz_val arg1, jz_val arg2)
      Takes two arguments.

   ...

   8: (JZ_STATE, jz_args* args, jz_val arg1, jz_val arg2, jz_val arg3,
       jz_val arg4, jz_val arg5, jz_val arg6, jz_val arg7, jz_val arg8)
      Takes eight arguments.

   Note that arity can be at most 8.
   Functions that need more arguments than that
   can declare arity -1 and manually extract the arguments from the array.

   Note also that if the arity property of a function
   should be different than the arity given here,
   one can set that property manually on the function object. */
typedef jz_val jz_fn();

typedef struct jz_args {
  jz_obj* callee;
  unsigned int length;
  const jz_val* argv;
} jz_args;

typedef struct jz_func_data {
  int arity;
  jz_bytecode* code;
  jz_closure_locals* scope;
  jz_val** closure_vars;
} jz_func_data;

#define JZ_ARITY_VAR -1

/* TODO: Assert that func is a function */
#define JZ_FUNC_DATA(func) ((jz_func_data*)((func)->data))

jz_val jz_call_arr(JZ_STATE, jz_obj* func, int argc, const jz_val* argv);

jz_obj* jz_func_new(JZ_STATE, jz_bytecode* code, int arity);
void jz_func_set_scope(JZ_STATE, jz_obj* this, jz_frame* scope);

#define jz_def(jz, obj, name, fn, arity)                  \
  jz_obj_put2(jz, obj, name, jz_fn_to_obj(jz, fn, arity))

jz_obj* jz_fn_to_obj(JZ_STATE, jz_fn* fn, int arity);

void jz_init_func_proto(JZ_STATE);

#endif
