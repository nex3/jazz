#include <stdlib.h>

#include "state.h"

jz_state* jz_init() {
  jz_state* state = malloc(sizeof(jz_state));

  /* Assign value constants. */
  JZ_TVAL_SET_TYPE(state->undefined_val, jz_undef);
  JZ_TVAL_SET_TYPE(state->true_val, jz_bool);
  state->true_val.value.b = true;
  JZ_TVAL_SET_TYPE(state->false_val, jz_bool);
  state->true_val.value.b = false;

  return state;
}
