#include <stdlib.h>
#include <stdio.h>

#include "state.h"
#include "lex.h"
#include "object.h"
#include "function.h"

/* Default stack size is 1 MB */
#define STACK_SIZE (1 << 20)

static void init_prototypes(JZ_STATE);
static void init_global_object(JZ_STATE);

static jz_tvalue jz_print(JZ_STATE, jz_args* args, int argc, const jz_tvalue* argv);

jz_state* jz_init() {
  jz_state* state = malloc(sizeof(jz_state));

  state->stack = calloc(sizeof(jz_byte), STACK_SIZE);

  /* Assign value constants. */
  JZ_TVAL_SET_TYPE(state->undefined_val, jz_t_undef);
  JZ_TVAL_SET_TYPE(state->null_val, jz_t_obj);
  state->null_val.value.obj = NULL;
  JZ_TVAL_SET_TYPE(state->true_val, jz_t_bool);
  state->true_val.value.b = jz_true;
  JZ_TVAL_SET_TYPE(state->false_val, jz_t_bool);
  state->false_val.value.b = jz_false;

  state->current_frame = NULL;

  jz_gc_init(state);
  jz_lex_init(state);
  init_prototypes(state);
  init_global_object(state);

  return state;
}

void init_prototypes(JZ_STATE) {
  jz->prototypes = jz_obj_new_bare(jz);
  jz_init_obj_proto(jz);
  jz_init_func_proto(jz);
}

/* TODO: Free the global object
   TODO: Modify global object's [[Get]] so that it throws ReferenceErrors */
void init_global_object(JZ_STATE) {
  jz->global_obj = jz_obj_new(jz);
  jz_obj_put2(jz, jz->global_obj, "NaN", jz_wrap_num(jz, JZ_NAN));
  jz_obj_put2(jz, jz->global_obj, "Infinity", jz_wrap_num(jz, JZ_INF));

  /* This is an extension to ECMAscript.
     The specification doesn't require support for a global undefined property. */
  jz_obj_put2(jz, jz->global_obj, "undefined", JZ_UNDEFINED);

  /* TODO: Move this to general lib initializer
     (Probably not even here - in main or something) */
  jz_def(jz, jz->global_obj, "print", jz_print, JZ_ARITY_VAR);
}

void jz_free_state(JZ_STATE) {
  jz_lex_free(jz);
  jz->global_obj = NULL;
  jz->prototypes = NULL;
  jz_gc_cycle(jz);
  free(jz);
}

/* TODO: This definitely doesn't belong here.
   Move it to src/io.c? lib/io.c? */
jz_tvalue jz_print(JZ_STATE, jz_args* args, int argc, const jz_tvalue* argv) {
  int i;

  for (i = 0; i < argc; i++) {
    char* str = jz_str_to_chars(jz, jz_to_str(jz, argv[i]));
    
    printf("%s\n", str);
    free(str);
  }

  return JZ_UNDEFINED;
}
