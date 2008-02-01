#ifndef JZ_OPCODE_H
#define JZ_OPCODE_H

#include "value.h"

#include <stdlib.h>

typedef unsigned char jz_opcode;
typedef unsigned short int jz_index;

typedef enum {
  jz_oc_push_literal,
  jz_oc_jump,
  jz_oc_jump_unless,
  jz_oc_jump_if,
  jz_oc_store,
  jz_oc_index_store,
  jz_oc_store_global,
  jz_oc_retrieve,
  jz_oc_index,
  jz_oc_load_global,
  jz_oc_pop,
  jz_oc_dup,
  jz_oc_dup2,
  jz_oc_rot4,
  jz_oc_bw_or,
  jz_oc_xor,
  jz_oc_bw_and,
  jz_oc_equals,
  jz_oc_strict_eq,
  jz_oc_lt,
  jz_oc_gt,
  jz_oc_lt_eq,
  jz_oc_gt_eq,
  jz_oc_lshift,
  jz_oc_rshift,
  jz_oc_urshift,
  jz_oc_add,
  jz_oc_sub,
  jz_oc_times,
  jz_oc_div,
  jz_oc_mod,
  jz_oc_to_num,
  jz_oc_neg,
  jz_oc_bw_not,
  jz_oc_not,
  jz_oc_ret,
  jz_oc_end,
  jz_oc_noop
} jz_oc_type;

#define JZ_OCS_TVALUE  (sizeof(jz_tvalue)/sizeof(jz_opcode))
#define JZ_OCS_PTRDIFF (sizeof(ptrdiff_t)/sizeof(jz_opcode))
#define JZ_OCS_INDEX   (sizeof(jz_index)/sizeof(jz_opcode))

#endif
