#ifndef JZ_VM_H
#define JZ_VM_H

typedef unsigned char jz_opcode;
typedef char jz_oc_arg;

typedef enum {
  jz_oc_push_double,
  jz_oc_add,
  jz_oc_ret
} jz_oc_type;

double jz_vm_run(jz_opcode* code, int stack_size);

#endif
