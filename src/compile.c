#include "compile.h"

#include <string.h>
#include <assert.h>

#define CODE_LENGTH_START 100
#define CODE_LENGTH_MULT  1.5

static void compile_node(jz_bytecode* bytecode, jz_parse_node* parse_node);
static void compile_number(jz_bytecode* bytecode, jz_parse_node* number_node);

static void push_opcode(jz_bytecode* bytecode, jz_opcode code);
static void push_multibyte_arg(jz_bytecode* bytecode, void* data, size_t size);

static void resize_code(jz_bytecode* bytecode);

jz_bytecode* jz_compile(jz_parse_node* parse_tree) {
  jz_bytecode* bytecode = malloc(sizeof(jz_bytecode));

  bytecode->code = calloc(sizeof(jz_opcode), CODE_LENGTH_START);
  bytecode->code_top = bytecode->code;
  bytecode->code_length = CODE_LENGTH_START;
  bytecode->stack_length = 0;

  compile_node(bytecode, parse_tree);
  push_opcode(bytecode, jz_oc_ret);

  bytecode->code_top = bytecode->code;
  return bytecode;
}

void compile_node(jz_bytecode* bytecode, jz_parse_node* parse_node) {
  switch (parse_node->type) {
  case jz_parse_num:
    compile_number(bytecode, parse_node);
    break;
  default:
    assert(0);
    break;
  }
}

void compile_number(jz_bytecode* bytecode, jz_parse_node* number_node) {
  bytecode->stack_length++;
  push_opcode(bytecode, jz_oc_push_double);
  push_multibyte_arg(bytecode, &(number_node->car.num), JZ_OCS_DOUBLE);
}

void push_opcode(jz_bytecode* bytecode, jz_opcode code) {
  if (bytecode->code_top - bytecode->code == bytecode->code_length)
    resize_code(bytecode);
  assert(bytecode->code_top - bytecode->code < bytecode->code_length);

  *(bytecode->code_top) = code;
  bytecode->code_top++;
}

void push_multibyte_arg(jz_bytecode* bytecode, void* data, size_t size) {
  while (bytecode->code_top - bytecode->code + size > bytecode->code_length)
    resize_code(bytecode);
  assert(bytecode->code_top - bytecode->code < bytecode->code_length);

  memcpy(bytecode->code_top, data, size);
  bytecode->code_top += size;
}

void resize_code(jz_bytecode* bytecode) {
  jz_opcode* old_code = bytecode->code;
  jz_opcode* old_top = bytecode->code_top;
  size_t old_length = bytecode->code_length;

  bytecode->code_length *= CODE_LENGTH_MULT;
  bytecode->code = calloc(sizeof(jz_opcode), bytecode->code_length);
  memcpy(bytecode->code, old_code, old_length);
  bytecode->code_top = bytecode->code + (old_top - old_code);
  free(old_code);
}
