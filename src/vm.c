#include "vm.h"
#include "frame.h"
#include "state.h"
#include "string.h"
#include "gc.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#define NEXT_OPCODE (*((code)++))
#define READ_ARG_INTO(type, var)                \
  type var = *(type*)(code);                    \
  code += sizeof(type)/sizeof(jz_opcode);

#define POP()     (*(--stack))
#define PUSH(val) (*(stack++) = (val))
#define PUSH_WB(val) {                          \
    jz_tvalue tmp = (val);                      \
    *(stack++) = tmp;                           \
    if (JZ_GC_WRITE_BARRIER_ACTIVE(jz))         \
      JZ_GC_MARK_VAL_GRAY(jz, tmp);             \
  }

#define STACK_SET(i, val) (stack[(i)] = (val))
#define STACK_SET_WB(i, val) {                  \
    jz_tvalue tmp = (val);                      \
    stack[(i)] = tmp;                           \
    if (JZ_GC_WRITE_BARRIER_ACTIVE(jz))         \
      JZ_GC_MARK_VAL_GRAY(jz, tmp);             \
  }

#if JZ_DEBUG_BYTECODE
static void print_bytecode(const jz_bytecode* bytecode);
#endif

typedef struct foo foo;

int foobar(foo* blat) {
  return 1 + 1;
}

jz_tvalue jz_vm_run(JZ_STATE, const jz_bytecode* bytecode) {
  jz_opcode* code = bytecode->code;
  jz_frame* frame = jz_frame_new(jz, bytecode);
  jz_tvalue* stack = JZ_FRAME_STACK(frame);
  jz_tvalue* locals = JZ_FRAME_LOCALS(frame);
  jz_tvalue* consts = bytecode->consts;

  frame->stack_top = &stack;
  jz->current_frame = frame;

#if JZ_DEBUG_BYTECODE
  print_bytecode(bytecode);
  printf("Stack length: %d\n", bytecode->stack_length);
  printf("Locals length: %d\n", bytecode->locals_length);
#endif

  while (true) {
    jz_gc_tick(jz);

    switch (NEXT_OPCODE) {
    case jz_oc_push_literal: {
      READ_ARG_INTO(jz_index, index);
      PUSH(consts[index]);
      break;
    }

    case jz_oc_jump: {
      READ_ARG_INTO(ptrdiff_t, jump);
      code += jump;
      break;
    }

    case jz_oc_jump_unless: {
      READ_ARG_INTO(ptrdiff_t, jump);
      if (!jz_to_bool(jz, POP())) code += jump;
      break;
    }

    case jz_oc_jump_if: {
      READ_ARG_INTO(ptrdiff_t, jump);
      if (jz_to_bool(jz, POP())) code += jump;
      break;
    }

    case jz_oc_store: {
      READ_ARG_INTO(jz_index, index);
      locals[index] = POP();
      break;
    }

    case jz_oc_retrieve: {
      READ_ARG_INTO(jz_index, index);
      PUSH(locals[index]);
      break;
    }

    case jz_oc_pop:
      stack--;
      break;

    case jz_oc_dup: {
      *stack = stack[-1];
      stack++;
      break;
    }

    case jz_oc_bw_or:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_int32(jz, stack[-2]) |
                                jz_to_int32(jz, stack[-1])));
      stack--;
      break;

    case jz_oc_xor:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_int32(jz, stack[-2]) ^
                                jz_to_int32(jz, stack[-1])));
      stack--;
      break;

    case jz_oc_bw_and:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_int32(jz, stack[-2]) &
                                jz_to_int32(jz, stack[-1])));
      stack--;
      break;

    case jz_oc_equals:
      STACK_SET(-2, jz_wrap_bool(jz, jz_values_equal(jz, stack[-2],
                                                     stack[-1])));
      stack--;
      break;

    case jz_oc_strict_eq:
      STACK_SET(-2, jz_wrap_bool(jz, jz_values_strict_equal(jz, stack[-2],
                                                            stack[-1])));
      stack--;
      break;

    case jz_oc_lt: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET(-2, jz_wrap_bool(jz, false));
      else STACK_SET(-2, jz_wrap_bool(jz, comp < 0));

      stack--;
      break;
    }

    case jz_oc_gt: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET(-2, jz_wrap_bool(jz, false));
      else STACK_SET(-2, jz_wrap_bool(jz, comp > 0));

      stack--;
      break;
    }

    case jz_oc_lt_eq: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET(-2, jz_wrap_bool(jz, false));
      else STACK_SET(-2, jz_wrap_bool(jz, comp <= 0));

      stack--;
      break;
    }

    case jz_oc_gt_eq: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET(-2, jz_wrap_bool(jz, false));
      else STACK_SET(-2, jz_wrap_bool(jz, comp >= 0));

      stack--;
      break;
    }

    case jz_oc_lshift:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_int32(jz, stack[-2]) <<
                                (jz_to_uint32(jz, stack[-1]) & 0x1F)));
      stack--;
      break;

    case jz_oc_rshift:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_int32(jz, stack[-2]) >>
                                (jz_to_uint32(jz, stack[-1]) & 0x1F)));
      stack--;
      break;

    case jz_oc_urshift:
      STACK_SET(-2, jz_wrap_num(jz, (unsigned int)jz_to_int32(jz, stack[-2]) >>
                                (jz_to_uint32(jz, stack[-1]) & 0x1F)));
      stack--;
      break;

    case jz_oc_add: {
      jz_tvalue v1 = stack[-2];
      jz_tvalue v2 = stack[-1];

      if (JZ_TVAL_TYPE(v1) == jz_strt || JZ_TVAL_TYPE(v2) == jz_strt) {
        STACK_SET_WB(-2, jz_wrap_str(jz, jz_str_concat(jz, jz_to_str(jz, v1),
                                                       jz_to_str(jz, v2))));
      } else {
        STACK_SET(-2, jz_wrap_num(jz, jz_to_num(jz, stack[-2]) +
                                  jz_to_num(jz, stack[-1])));
      }
      stack--;
      break;
    }

    case jz_oc_sub:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_num(jz, stack[-2]) -
                                jz_to_num(jz, stack[-1])));
      stack--;
      break;

    case jz_oc_times:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_num(jz, stack[-2]) *
                                jz_to_num(jz, stack[-1])));
      stack--;
      break;

    case jz_oc_div:
      STACK_SET(-2, jz_wrap_num(jz, jz_to_num(jz, stack[-2]) /
                                jz_to_num(jz, stack[-1])));
      stack--;
      break;

    case jz_oc_mod:
      STACK_SET(-2, jz_wrap_num(jz, jz_num_mod(jz, stack[-2], stack[-1])));
      stack--;
      break;

    case jz_oc_to_num:
      STACK_SET(-1, jz_wrap_num(jz, jz_to_num(jz, stack[-1])));
      break;

    case jz_oc_neg:
      STACK_SET(-1, jz_wrap_num(jz, -jz_to_num(jz, stack[-1])));
      break;

    case jz_oc_bw_not:
      STACK_SET(-1, jz_wrap_num(jz, ~jz_to_int32(jz, stack[-1])));
      break;

    case jz_oc_not:
      STACK_SET(-1, jz_wrap_bool(jz, !jz_to_bool(jz, stack[-1])));
      break;

    case jz_oc_ret: {
      jz_tvalue res;

      res = stack[-1];
      jz_frame_free(jz, frame);
      jz->current_frame = NULL;
      return res;
    }

    case jz_oc_end:
      jz_frame_free(jz, frame);
      jz->current_frame = NULL;
      return JZ_UNDEFINED;

    default:
      fprintf(stderr, "Unknown opcode %d\n", code[-1]);
      exit(1);
    }
  }
}

#if JZ_DEBUG_BYTECODE
void print_bytecode(const jz_bytecode* bytecode) {
  jz_opcode* code;

  printf("Bytecode:\n");
  for (code = bytecode->code;
       code - bytecode->code < bytecode->code_length; code++) {
    char* name = "???";
    size_t argsize = 0;

    switch (*code) {
    case jz_oc_push_literal:
      name = "push_literal";
      argsize = JZ_OCS_INDEX;
      break;

    case jz_oc_jump:
      name = "jump";
      argsize = JZ_OCS_PTRDIFF;
      break;

    case jz_oc_jump_unless:
      name = "jump_unless";
      argsize = JZ_OCS_PTRDIFF;
      break;

    case jz_oc_jump_if:
      name = "jump_if";
      argsize = JZ_OCS_PTRDIFF;
      break;

    case jz_oc_store:
      name = "store";
      argsize = JZ_OCS_INDEX;
      break;

    case jz_oc_retrieve:
      name = "retrieve";
      argsize = JZ_OCS_INDEX;
      break;

    case jz_oc_pop:
      name = "pop";
      break;

    case jz_oc_dup:
      name = "dup";
      break;

    case jz_oc_bw_or:
      name = "bw_or";
      break;

    case jz_oc_xor:
      name = "xor";
      break;

    case jz_oc_bw_and:
      name = "bw_and";
      break;

    case jz_oc_equals:
      name = "equals";
      break;

    case jz_oc_strict_eq:
      name = "strict_eq";
      break;

    case jz_oc_lt:
      name = "lt";
      break;

    case jz_oc_gt:
      name = "gt";
      break;

    case jz_oc_lt_eq:
      name = "lt_eq";
      break;

    case jz_oc_gt_eq:
      name = "gt_eq";
      break;

    case jz_oc_lshift:
      name = "lshift";
      break;

    case jz_oc_rshift:
      name = "rshift";
      break;

    case jz_oc_urshift:
      name = "urshift";
      break;

    case jz_oc_add:
      name = "add";
      break;

    case jz_oc_sub:
      name = "sub";
      break;

    case jz_oc_times:
      name = "times";
      break;

    case jz_oc_div:
      name = "div";
      break;

    case jz_oc_mod:
      name = "mod";
      break;

    case jz_oc_to_num:
      name = "to_num";
      break;

    case jz_oc_neg:
      name = "neg";
      break;

    case jz_oc_bw_not:
      name = "bw_not";
      break;

    case jz_oc_not:
      name = "not";
      break;

    case jz_oc_ret:
      name = "ret";
      break;

    case jz_oc_end:
      name = "end";
      break;
    }

    printf("%d: %s (%d)\n", code - bytecode->code, name, argsize);
    code += argsize;
  }
}
#endif
