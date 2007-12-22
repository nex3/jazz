#ifndef JZ_COMPILE_H
#define JZ_COMPILE_H

#include "parse.h"
#include "opcode.h"

typedef struct {
  jz_opcode* code;
  int stack_size;
} jz_bytecode;

#endif
