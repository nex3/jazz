#include <stdlib.h>
#include <stdio.h>

#include "state.h"
#include "lex.h"
#include "object.h"

static void init_global_object(JZ_STATE);

jz_state* jz_init() {
  jz_state* state = malloc(sizeof(jz_state));

  /* Assign value constants. */
  JZ_TVAL_SET_TYPE(state->undefined_val, jz_t_undef);
  JZ_TVAL_SET_TYPE(state->null_val, jz_t_obj);
  state->null_val.value.obj = NULL;
  JZ_TVAL_SET_TYPE(state->true_val, jz_t_bool);
  state->true_val.value.b = true;
  JZ_TVAL_SET_TYPE(state->false_val, jz_t_bool);
  state->true_val.value.b = false;

  state->current_frame = NULL;

  jz_gc_init(state);
  jz_lex_init(state);
  init_global_object(state);

  return state;
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
}

void jz_free_state(JZ_STATE) {
  jz_lex_free(jz);
  free(jz);
}
