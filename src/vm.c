#include "vm.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define NEXT_OPCODE (*((code)++))
#define OC_ARG      ((jz_oc_arg)(*code))

#define PUSH(val)  (*(stack++) = (val))

double jz_vm_run(jz_opcode* code, int stack_size) {
  double* stack = calloc(sizeof(double), stack_size);
  double* stack_bottom = stack;
  double res;

  while (true) {
    switch (NEXT_OPCODE) {
    case jz_oc_push_double:
      PUSH(*(double*)(code));
      code += sizeof(double)/sizeof(jz_opcode);
      break;

    case jz_oc_add:
      res = stack[-1] + stack[-2];
      stack--;
      *stack = res;
      break;

    case jz_oc_ret:
      res = *stack;
      free(stack_bottom);
      return res;
    }
  }
}
