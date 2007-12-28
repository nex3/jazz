#include "vm.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#define NEXT_OPCODE (*((code)++))
#define OC_ARG      ((jz_oc_arg)(*code))
#define READ_ARG_INTO(type, var)                \
  type var = *(type*)(code);                    \
  code += sizeof(type)/sizeof(jz_opcode);

#define POP        (*(--stack))
#define PUSH(val)  (*(stack++) = (val))

#if JZ_DEBUG_BYTECODE
static void print_bytecode(jz_bytecode* bytecode);
#endif

jz_tvalue jz_vm_run(jz_bytecode* bytecode) {
  jz_opcode* code = bytecode->code;
  jz_tvalue* stack = calloc(sizeof(jz_tvalue), bytecode->stack_length);
  jz_tvalue* stack_bottom = stack;
  jz_tvalue* locals = calloc(sizeof(jz_tvalue), bytecode->locals_length);

#if JZ_DEBUG_BYTECODE
  print_bytecode(bytecode);
#endif

  while (true) {
    switch (NEXT_OPCODE) {
    case jz_oc_push_literal: {
      READ_ARG_INTO(jz_tvalue, literal);
      PUSH(literal);
      break;
    }

    case jz_oc_jump: {
      READ_ARG_INTO(size_t, jump);
      code += jump;
      break;
    }

    case jz_oc_jump_if: {
      READ_ARG_INTO(size_t, jump);
      if (!jz_to_bool(POP)) code += jump;
      break;
    }

    case jz_oc_store: {
      READ_ARG_INTO(unsigned char, index);
      locals[index] = POP;
      break;
    }

    case jz_oc_retrieve: {
      READ_ARG_INTO(unsigned char, index);
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
      stack[-2] = jz_wrap_num(jz_to_int32(stack[-2]) | jz_to_int32(stack[-1]));
      stack--;
      break;

    case jz_oc_xor:
      stack[-2] = jz_wrap_num(jz_to_int32(stack[-2]) ^ jz_to_int32(stack[-1]));
      stack--;
      break;

    case jz_oc_bw_and:
      stack[-2] = jz_wrap_num(jz_to_int32(stack[-2]) & jz_to_int32(stack[-1]));
      stack--;
      break;

    case jz_oc_equal:
      stack[-2] = jz_wrap_bool(jz_values_equal(stack[-2], stack[-1]));
      stack--;
      break;

    case jz_oc_strict_equal:
      stack[-2] = jz_wrap_bool(jz_values_strict_equal(stack[-2], stack[-1]));
      stack--;
      break;

    case jz_oc_lt: {
      double comp = jz_values_comp(stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) stack[-2] = jz_wrap_bool(false);
      else stack[-2] = jz_wrap_bool(comp < 0);

      stack--;
      break;
    }

    case jz_oc_gt: {
      double comp = jz_values_comp(stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) stack[-2] = jz_wrap_bool(false);
      else stack[-2] = jz_wrap_bool(comp > 0);

      stack--;
      break;
    }

    case jz_oc_lte: {
      double comp = jz_values_comp(stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) stack[-2] = jz_wrap_bool(false);
      else stack[-2] = jz_wrap_bool(comp <= 0);

      stack--;
      break;
    }

    case jz_oc_gte: {
      double comp = jz_values_comp(stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) stack[-2] = jz_wrap_bool(false);
      else stack[-2] = jz_wrap_bool(comp >= 0);

      stack--;
      break;
    }

    case jz_oc_lshift:
      stack[-2] = jz_wrap_num(jz_to_int32(stack[-2]) <<
                              (jz_to_uint32(stack[-1]) & 0x1F));
      stack--;
      break;

    case jz_oc_rshift:
      stack[-2] = jz_wrap_num(jz_to_int32(stack[-2]) >>
                              (jz_to_uint32(stack[-1]) & 0x1F));
      stack--;
      break;

    case jz_oc_urshift:
      stack[-2] = jz_wrap_num((unsigned int)jz_to_int32(stack[-2]) >>
                              (jz_to_uint32(stack[-1]) & 0x1F));
      stack--;
      break;

    case jz_oc_add:
      stack[-2] = jz_wrap_num(jz_to_num(stack[-2]) + jz_to_num(stack[-1]));
      stack--;
      break;

    case jz_oc_sub:
      stack[-2] = jz_wrap_num(jz_to_num(stack[-2]) - jz_to_num(stack[-1]));
      stack--;
      break;

    case jz_oc_times:
      stack[-2] = jz_wrap_num(jz_to_num(stack[-2]) * jz_to_num(stack[-1]));
      stack--;
      break;

    case jz_oc_div:
      stack[-2] = jz_wrap_num(jz_to_num(stack[-2]) / jz_to_num(stack[-1]));
      stack--;
      break;

    case jz_oc_mod:
      stack[-2] = jz_wrap_num(jz_num_mod(stack[-2], stack[-1]));
      stack--;
      break;

    case jz_oc_to_num:
      stack[-1] = jz_wrap_num(jz_to_num(stack[-1]));
      break;

    case jz_oc_neg:
      stack[-1] = jz_wrap_num(-jz_to_num(stack[-1]));
      break;

    case jz_oc_bw_not:
      stack[-1] = jz_wrap_num(~jz_to_int32(stack[-1]));
      break;

    case jz_oc_not:
      stack[-1] = jz_wrap_bool(!jz_to_bool(stack[-1]));
      break;

    case jz_oc_ret: {
      jz_tvalue res;

      res = stack[-1];
      free(stack_bottom);
      return res;
    }

    case jz_oc_end:
      free(stack_bottom);
      return jz_undef_val();

    default:
      fprintf(stderr, "Unknown opcode %d\n", code[-1]);
      exit(1);
    }
  }
}

#if JZ_DEBUG_BYTECODE
void print_bytecode(jz_bytecode* bytecode) {
  jz_opcode* code;

  printf("Bytecode:\n");
  for (code = bytecode->code;
       code - bytecode->code < bytecode->code_length; code++) {
    char* name = "???";
    size_t argsize = 0;

    switch (*code) {
    case jz_oc_push_literal:
      name = "push_literal";
      argsize = JZ_OCS_TVALUE;
      break;

    case jz_oc_jump:
      name = "jump";
      argsize = JZ_OCS_SIZET;
      break;

    case jz_oc_jump_if:
      name = "jump_if";
      argsize = JZ_OCS_SIZET;
      break;

    case jz_oc_store:
      name = "store";
      argsize = 1;
      break;

    case jz_oc_retrieve:
      name = "retrieve";
      argsize = 1;
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

    case jz_oc_equal:
      name = "equal";
      break;

    case jz_oc_strict_equal:
      name = "strict_equal";
      break;

    case jz_oc_lt:
      name = "lt";
      break;

    case jz_oc_gt:
      name = "gt";
      break;

    case jz_oc_lte:
      name = "lte";
      break;

    case jz_oc_gte:
      name = "gte";
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
