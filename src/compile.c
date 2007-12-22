#include "compile.h"

#include <string.h>
#include <assert.h>

#define CODE_LENGTH_START 100
#define CODE_LENGTH_MULT  1.5

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define CDAR(node) ((node)->cdr.node->car)
#define CDDR(node) ((node)->cdr.node->cdr)

static void compile_node(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_binop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_number(jz_bytecode* bytecode, jz_parse_node* node);

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

void compile_node(jz_bytecode* bytecode, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_num:
    compile_number(bytecode, node);
    break;
  case jz_parse_binop:
    compile_binop(bytecode, node);
    break;
  default:
    assert(0);
    break;
  }
}

void compile_binop(jz_bytecode* bytecode, jz_parse_node* node) {
  int left_cap, right_cap;

  compile_node(bytecode, CDAR(node).node);
  left_cap = bytecode->stack_length;

  compile_node(bytecode, CDDR(node).node);
  right_cap = bytecode->stack_length;

  switch (node->car.op_type) {
  case jz_op_plus:
    push_opcode(bytecode, jz_oc_add);
    break;
  default: assert(0);
  }

  bytecode->stack_length = MAX(left_cap, right_cap + 1);
}

void compile_number(jz_bytecode* bytecode, jz_parse_node* node) {
  bytecode->stack_length = 1;
  push_opcode(bytecode, jz_oc_push_double);
  push_multibyte_arg(bytecode, &(node->car.num), JZ_OCS_DOUBLE);
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
