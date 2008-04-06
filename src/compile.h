/* The Jazz compiler.
   Translates a parse tree (see parse.h)
   into bytecode to be executed by the virtual machine (see vm.h). */

#ifndef JZ_COMPILE_H
#define JZ_COMPILE_H

#include "jazz.h"
#include "parse.h"
#include "opcode.h"
#include "vector.h"
#include "cons.h"

typedef struct {
  jz_opcode* code;
  size_t code_length;
  size_t locals_length;
  size_t closure_vars_length;
  size_t closure_locals_length;
  jz_byte* param_locs;
  jz_val* consts;
  size_t consts_length;
} jz_bytecode;

JZ_DECLARE_VECTOR(jz_opcode)

#define JZ_BITFIELD_SET(field, i, val) ((field)[(i) / 8] |= 1 << ((i) % 8))
#define JZ_BITFIELD_GET(field, i)      ((field)[(i) / 8] &  1 << ((i) % 8))

/* Compiles a parse tree into Jazz bytecode.
   The caller is responsible for freeing the returned bytecode
   using jz_free_bytecode. */
jz_bytecode* jz_compile(JZ_STATE, jz_cons* parse_tree);

/* Frees a jz_bytecode*.
   Does nothing if 'this' is NULL. */
void jz_free_bytecode(JZ_STATE, jz_bytecode* this);

void jz_mark_bytecode(JZ_STATE, const jz_bytecode* this);

#endif
