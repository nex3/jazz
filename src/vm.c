#include "vm.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define NEXT_OPCODE (*((code)++))
#define OC_ARG      ((jz_oc_arg)(*code))

#define PUSH(val)  (*(stack++) = (val))

jz_tvalue jz_vm_run(jz_bytecode* bytecode) {
  jz_opcode* code = bytecode->code;
  jz_tvalue* stack = calloc(sizeof(jz_tvalue), bytecode->stack_length);
  jz_tvalue* stack_bottom = stack;
  jz_tvalue res;

  while (true) {
    switch (NEXT_OPCODE) {
    case jz_oc_push_literal:
      PUSH(*(jz_tvalue*)(code));
      code += JZ_OCS_TVALUE;
      break;

    case jz_oc_add:
      res = jz_wrap_num(stack[-2].value.num + stack[-1].value.num);
      stack--;
      stack[-1] = res;
      break;

    case jz_oc_sub:
      res = jz_wrap_num(stack[-2].value.num - stack[-1].value.num);
      stack--;
      stack[-1] = res;
      break;

    case jz_oc_times:
      res = jz_wrap_num(stack[-2].value.num * stack[-1].value.num);
      stack--;
      stack[-1] = res;
      break;

    case jz_oc_div:
      res = jz_wrap_num(stack[-2].value.num / stack[-1].value.num);
      stack--;
      stack[-1] = res;
      break;

    case jz_oc_ret:
      res = stack[-1];
      free(stack_bottom);
      return res;
    }
  }
}
