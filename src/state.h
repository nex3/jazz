#ifndef JZ_STATE_H
#define JZ_STATE_H

#include "type.h"

typedef struct {
  jz_tvalue undefined_val;
  jz_tvalue true_val;
  jz_tvalue false_val;
} jz_state;

#define JZ_STATE jz_state* jstate

#define JZ_UNDEFINED (jstate->undefined_val)
#define JZ_TRUE      (jstate->true_val)
#define JZ_FALSE     (jstate->false_val)

jz_state* jz_init();

#endif
