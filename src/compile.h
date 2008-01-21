/* The Jazz compiler.
   Translates a parse tree (see parse.h)
   into bytecode to be executed by the virtual machine (see vm.h). */

#ifndef JZ_COMPILE_H
#define JZ_COMPILE_H

#include "state.h"
#include "parse.h"
#include "opcode.h"
#include "vector.h"

JZ_DECLARE_VECTOR(jz_opcode)

/* This struct contains bytecode for the virtual machine to execute,
   as well as enough metadata to know how to execute it properly. */
typedef struct {
  jz_opcode* code;
  size_t code_length;
  size_t stack_length;
  size_t locals_length;
  jz_tvalue* consts;
} jz_bytecode;

/* Compiles a parse tree into Jazz bytecode.
   The caller is responsible for freeing the returned bytecode
   using jz_free_bytecode. */
jz_bytecode* jz_compile(JZ_STATE, jz_parse_node* parse_tree);

/* Frees a jz_bytecode*.
   Does nothing if 'this' is NULL. */
void jz_free_bytecode(JZ_STATE, jz_bytecode* this);

void jz_free_parse_tree(JZ_STATE, jz_parse_node* root);

#endif
