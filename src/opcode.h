#ifndef JZ_OPCODE_H
#define JZ_OPCODE_H

#include "type.h"

#include <stdlib.h>

typedef unsigned char jz_opcode;
typedef char jz_oc_arg;

typedef enum {
  jz_oc_push_literal,
  jz_oc_jump,
  jz_oc_jump_if,
  jz_oc_dup,
  jz_oc_bw_or,
  jz_oc_xor,
  jz_oc_bw_and,
  jz_oc_equal,
  jz_oc_add,
  jz_oc_sub,
  jz_oc_times,
  jz_oc_div,
  jz_oc_not,
  jz_oc_ret
} jz_oc_type;

#define JZ_OCS_TVALUE (sizeof(jz_tvalue)/sizeof(jz_opcode))
#define JZ_OCS_SIZET  (sizeof(size_t)/sizeof(jz_opcode))

#endif
