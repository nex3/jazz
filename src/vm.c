#include "vm.h"
#include "frame.h"
#include "state.h"
#include "string.h"
#include "gc.h"
#include "object.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

const char* jz_oc_names[] = {
  "jump", "jump_unless", "jump_if", "store_global", "retrieve",
  "store", "closure_retrieve", "closure_store", "load_global", "call",
  "push_literal", "push_closure", "push_global", "index",
  "index_store", "pop", "dup", "dup2", "rot4", "bw_or", "xor",
  "bw_and", "equals", "strict_eq", "lt", "gt", "lt_eq", "gt_eq",
  "lshift", "rshift", "urshift", "add", "sub", "times", "div", "mod",
  "to_num", "neg", "bw_not", "not", "ret", "end", "noop"
};

#define NEXT_OPCODE (*((code)++))
#define READ_ARG_INTO(type, var)                \
  type var = *(type*)(code);                    \
  code += sizeof(type)/sizeof(jz_opcode);

#define POP()     (*(--stack))
#define PUSH_NO_WB(val) (*(stack++) = (val))
#define PUSH(val) {                             \
    jz_val tmp = (val);                         \
    *(stack++) = tmp;                           \
    if (jz_gc_write_barrier_active(jz))         \
      JZ_GC_MARK_VAL_GRAY(jz, tmp);             \
  }

#define STACK_SET_NO_WB(i, val) (stack[(i)] = (val))
#define STACK_SET(i, val) {                     \
    jz_val tmp = (val);                         \
    stack[(i)] = tmp;                           \
    if (jz_gc_write_barrier_active(jz))         \
      JZ_GC_MARK_VAL_GRAY(jz, tmp);             \
  }

#if JZ_DEBUG_BYTECODE
static void print_bytecode(const jz_bytecode* bytecode);
#endif

jz_val jz_vm_run(JZ_STATE, const jz_bytecode* bytecode) {
  return jz_vm_run_frame(jz, jz_frame_new(jz, bytecode));
}

jz_val jz_vm_run_frame(JZ_STATE, jz_frame* frame) {
  jz_opcode* code = frame->bytecode->code;
  jz_val* stack = JZ_FRAME_STACK(frame);
  jz_val** closure_vars = JZ_FRAME_CLOSURE_VARS(frame);
  jz_val* locals = JZ_FRAME_LOCALS(frame);
  jz_val* consts = frame->bytecode->consts;

  frame->stack_top = &stack;

#if JZ_DEBUG_BYTECODE
  print_bytecode(frame->bytecode);
  printf("Locals length: %lu\n", (unsigned long)frame->bytecode->locals_length);
#endif

  while (jz_true) {
    jz_gc_tick(jz);

    switch (NEXT_OPCODE) {
    case jz_oc_push_literal: {
      READ_ARG_INTO(jz_index, index);
      PUSH_NO_WB(consts[index]);
      break;
    }

    case jz_oc_push_closure: {
      jz_func_data* func;
      READ_ARG_INTO(jz_index, index);
      PUSH_NO_WB(consts[index]);

      assert(JZ_VAL_TYPE(consts[index]) == jz_t_obj);
      func = JZ_FUNC_DATA((jz_obj*)consts[index]);
      func->scope = frame->closure_locals;
      break;
    }

    case jz_oc_push_global: {
      PUSH_NO_WB(jz->global_obj);
      break;
    }

    case jz_oc_call: {
      jz_val obj;
      jz_val v;
      READ_ARG_INTO(jz_index, argc);

      obj = stack[-argc - 1];

      if (JZ_VAL_TYPE(obj) != jz_t_obj) {
        fprintf(stderr, "TypeError: %s is not an object.\n",
                jz_str_to_chars(jz, jz_to_str(jz, obj)));
        exit(1);
      }

      v = stack[-argc];

      jz->stack = (jz_byte*)stack;
      STACK_SET(-argc - 1, jz_call_arr(jz, (jz_obj*)obj, argc, stack - argc));
      stack -= argc;
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

    case jz_oc_index_store:
      jz_obj_put(jz, jz_to_obj(jz, stack[-3]),
                 jz_to_str(jz, stack[-2]), stack[-1]);

      stack -= 3;
      break;

    case jz_oc_store_global: {
      READ_ARG_INTO(jz_index, index);

      jz_obj_put(jz, jz->global_obj, jz_to_str(jz, consts[index]), POP());
      break;
    }

    case jz_oc_closure_store: {
      READ_ARG_INTO(jz_index, index);
      *(closure_vars[index]) = POP();
      break;
    }

    case jz_oc_retrieve: {
      READ_ARG_INTO(jz_index, index);
      PUSH_NO_WB(locals[index]);
      break;
    }

    case jz_oc_index:
      if (JZ_VAL_TYPE(stack[-2]) != jz_t_obj) {
        fprintf(stderr, "Indexing not yet implemented for non-object values.\n");
        exit(1);
      }
      STACK_SET(-2, jz_obj_get(jz, (jz_obj*)stack[-2], jz_to_str(jz, stack[-1])));
      stack--;
      break;

    case jz_oc_load_global: {
      READ_ARG_INTO(jz_index, index);

      PUSH(jz_obj_get(jz, jz->global_obj, jz_to_str(jz, consts[index])));
      break;
    }

    case jz_oc_closure_retrieve: {
      READ_ARG_INTO(jz_index, index);
      PUSH(*(closure_vars[index]));
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

    case jz_oc_dup2: {
      stack[0] = stack[-2];
      stack[1] = stack[-1];
      stack += 2;
      break;
    }

    case jz_oc_rot4: {
      jz_val tmp = stack[-1];

      stack[-1] = stack[-2];
      stack[-2] = stack[-3];
      stack[-3] = stack[-4];
      stack[-4] = tmp;
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
      STACK_SET_NO_WB(-2, jz_wrap_bool(jz, jz_values_equal(jz, stack[-2],
                                                           stack[-1])));
      stack--;
      break;

    case jz_oc_strict_eq: {
      STACK_SET_NO_WB(-2, jz_wrap_bool(jz, jz_values_strict_equal(jz, stack[-2],
                                                                  stack[-1])));
      stack--;
      break;
    }

    case jz_oc_lt: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET_NO_WB(-2, jz_wrap_bool(jz, jz_false));
      else STACK_SET_NO_WB(-2, jz_wrap_bool(jz, comp < 0));

      stack--;
      break;
    }

    case jz_oc_gt: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET_NO_WB(-2, jz_wrap_bool(jz, jz_false));
      else STACK_SET_NO_WB(-2, jz_wrap_bool(jz, comp > 0));

      stack--;
      break;
    }

    case jz_oc_lt_eq: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET_NO_WB(-2, jz_wrap_bool(jz, jz_false));
      else STACK_SET_NO_WB(-2, jz_wrap_bool(jz, comp <= 0));

      stack--;
      break;
    }

    case jz_oc_gt_eq: {
      double comp = jz_values_comp(jz, stack[-2], stack[-1]);

      if (JZ_NUM_IS_NAN(comp)) STACK_SET_NO_WB(-2, jz_wrap_bool(jz, jz_false));
      else STACK_SET_NO_WB(-2, jz_wrap_bool(jz, comp >= 0));

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
      jz_val v1 = stack[-2];
      jz_val v2 = stack[-1];

      if (JZ_VAL_TYPE(v1) == jz_t_str || JZ_VAL_TYPE(v2) == jz_t_str) {
        STACK_SET(-2, jz_str_concat(jz, jz_to_str(jz, v1),
                                    jz_to_str(jz, v2)));
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
      STACK_SET_NO_WB(-1, jz_wrap_bool(jz, !jz_to_bool(jz, stack[-1])));
      break;

    case jz_oc_ret: {
      jz_val res;

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

    jz_check_overflow(jz, (jz_byte*)stack);
  }
}

#if JZ_DEBUG_BYTECODE
void print_bytecode(const jz_bytecode* bytecode) {
  jz_opcode* code;

  printf("Bytecode:\n");
  for (code = bytecode->code;
       code - bytecode->code < bytecode->code_length; code++) {
    jz_opcode op = *code;
    size_t argsize = JZ_OC_ARGSIZE(op);

    printf("%4d:  %-15s (%lu)\n", code - bytecode->code, jz_oc_names[op],
           (unsigned long)argsize);
    code += argsize;
  }
}
#endif
