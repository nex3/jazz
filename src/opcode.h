#ifndef JZ_OPCODE_H
#define JZ_OPCODE_H

#include <stdlib.h>

typedef unsigned char jz_opcode;
typedef char jz_oc_arg;

typedef enum {
  jz_oc_push_double,
  jz_oc_add,
  jz_oc_sub,
  jz_oc_times,
  jz_oc_div,
  jz_oc_ret
} jz_oc_type;

#define JZ_OCS_DOUBLE (sizeof(double)/sizeof(jz_opcode))

#endif
