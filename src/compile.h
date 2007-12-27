#ifndef JZ_COMPILE_H
#define JZ_COMPILE_H

#include "parse.h"
#include "opcode.h"
#include "vector.h"

JZ_DECLARE_VECTOR(jz_opcode)

typedef struct {
  jz_opcode_vector* code;
  size_t stack_length;
} jz_bytecode;

jz_bytecode* jz_compile(jz_parse_node* parse_tree);

#endif
