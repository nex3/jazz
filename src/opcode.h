#ifndef JZ_OPCODE_H
#define JZ_OPCODE_H

typedef unsigned char jz_opcode;
typedef char jz_oc_arg;

typedef enum {
  jz_oc_push_double,
  jz_oc_add,
  jz_oc_ret
} jz_oc_type;

#endif
