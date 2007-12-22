#include "vm.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define NEXT_OPCODE (*((code)++))
#define OC_ARG      ((jz_oc_arg)(*code))

#define PUSH(val)  (*(stack++) = (val))

double jz_vm_run(jz_bytecode* bytecode) {
  jz_opcode* code = bytecode->code;
  double* stack = calloc(sizeof(double), bytecode->stack_length);
  double* stack_bottom = stack;
  double res;

  while (true) {
    switch (NEXT_OPCODE) {
    case jz_oc_push_double:
      PUSH(*(double*)(code));
      code += JZ_OCS_DOUBLE;
      break;

    case jz_oc_add:
      res = stack[-1] + stack[-2];
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
