#include <stdlib.h>
#include <stdio.h>

#include "state.h"
#include "lex.h"

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

  return state;
}

void jz_free_state(JZ_STATE) {
  jz_lex_free(jz);
  free(jz);
}
