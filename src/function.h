#ifndef JZ_FN_H
#define JZ_FN_H

#include "jazz.h"
#include "value.h"
#include "compile.h"

/* Note that the argument list for this type is empty
   to allow arbitary arguments to be passed.

   Which arguments it accepts are determined by its arity
   (set in jz_wrap_fn or jz_fn_to_obj):

   JZ_ARITY_VAR:
      (JZ_STATE, jz_args* args, int argc, const jz_tvalue* argv)
      Takes the function arguments as an array.

   0: (JZ_STATE, jz_args* args)
      Takes no arguments.

   1: (JZ_STATE, jz_args* args, jz_tvalue arg1)
      Takes one argument.

   2: (JZ_STATE, jz_args* args, jz_tvalue arg1, jz_tvalue arg2)
      Takes two arguments.

   ...

   8: (JZ_STATE, jz_args* args, jz_tvalue arg1, jz_tvalue arg2, jz_tvalue arg3,
       jz_tvalue arg4, jz_tvalue arg5, jz_tvalue arg6, jz_tvalue arg7, jz_tvalue arg8)
      Takes eight arguments.

   Note that arity can be at most 8.
   Functions that need more arguments than that
   can declare arity -1 and manually extract the arguments from the array.

   Note also that if the arity property of a function
   should be different than the arity given here,
   one can set that property manually on the function object. */
typedef jz_tvalue jz_fn();

typedef struct jz_args {
  jz_obj* callee;
  unsigned int length;
  const jz_tvalue* argv;
} jz_args;

jz_tvalue jz_call_arr(JZ_STATE, jz_obj* func, int argc, const jz_tvalue* argv);

#define JZ_ARITY_VAR -1

jz_obj* jz_func_new(JZ_STATE, jz_bytecode* code, int arity);

#define jz_def(jz, obj, name, fn, arity)                  \
  jz_obj_put2(jz, obj, name, jz_wrap_fn(jz, fn, arity))

#define jz_wrap_fn(jz, fn, arity) jz_wrap_obj(jz, jz_fn_to_obj(jz, fn, arity))
jz_obj* jz_fn_to_obj(JZ_STATE, jz_fn* fn, int arity);

void jz_init_func_proto(JZ_STATE);

#endif
