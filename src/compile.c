#include "compile.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#define CODE_LENGTH_START 100
#define CODE_LENGTH_MULT  1.5

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define CDAR(node) ((node)->cdr.node->car)
#define CDDR(node) ((node)->cdr.node->cdr)
#define CDDAR(node) (CDDR(node).node->car)
#define CDDDR(node) (CDDR(node).node->cdr)

static void compile_node(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_binop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_logical_binop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_simple_binop(jz_bytecode* bytecode, jz_parse_node* node, jz_opcode op);
static void compile_triop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_literal(jz_bytecode* bytecode, jz_parse_node* node);

static void jump_to_top_from(jz_bytecode* bytecode, size_t index);

static void push_opcode(jz_bytecode* bytecode, jz_opcode code);
static void push_multibyte_arg(jz_bytecode* bytecode, void* data, size_t size);
static size_t push_placeholder(jz_bytecode* bytecode, size_t size);

static void resize_code(jz_bytecode* bytecode);

jz_bytecode* jz_compile(jz_parse_node* parse_tree) {
  jz_bytecode* bytecode = malloc(sizeof(jz_bytecode));

  bytecode->code = calloc(sizeof(jz_opcode), CODE_LENGTH_START);
  bytecode->code_top = bytecode->code;
  bytecode->code_length = CODE_LENGTH_START;
  bytecode->stack_length = 0;

  compile_node(bytecode, parse_tree);
  push_opcode(bytecode, jz_oc_ret);

  bytecode->code_length = bytecode->code_top - bytecode->code;
  bytecode->code_top = bytecode->code;
  return bytecode;
}

void compile_node(jz_bytecode* bytecode, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_literal:
    compile_literal(bytecode, node);
    break;

  case jz_parse_binop:
    compile_binop(bytecode, node);
    break;

  case jz_parse_triop:
    compile_triop(bytecode, node);
    break;

  default:
    assert(0);
    break;
  }
}

void compile_binop(jz_bytecode* bytecode, jz_parse_node* node) {
  switch (node->car.op_type) {
  case jz_op_and:
  case jz_op_or:
    compile_logical_binop(bytecode, node);
    break;

  case jz_op_bw_or:
    compile_simple_binop(bytecode, node, jz_oc_bw_or);
    break;

  case jz_op_xor:
    compile_simple_binop(bytecode, node, jz_oc_xor);
    break;

  case jz_op_bw_and:
    compile_simple_binop(bytecode, node, jz_oc_bw_and);
    break;

  case jz_op_plus:
    compile_simple_binop(bytecode, node, jz_oc_add);
    break;

  case jz_op_minus:
    compile_simple_binop(bytecode, node, jz_oc_sub);
    break;

  case jz_op_times:
    compile_simple_binop(bytecode, node, jz_oc_times);
    break;

  case jz_op_div:
    compile_simple_binop(bytecode, node, jz_oc_div);
    break;

  default: assert(0);
  }
}

void compile_logical_binop(jz_bytecode* bytecode, jz_parse_node* node) {
  int left_cap, right_cap;
  size_t jump;

  compile_node(bytecode, CDAR(node).node);
  left_cap = bytecode->stack_length;
  push_opcode(bytecode, jz_oc_dup);

  if (node->car.op_type == jz_op_or) push_opcode(bytecode, jz_oc_not);

  push_opcode(bytecode, jz_oc_jump_if);
  jump = push_placeholder(bytecode, JZ_OCS_SIZET);

  compile_node(bytecode, CDDR(node).node);
  right_cap = bytecode->stack_length;
  jump_to_top_from(bytecode, jump);

  bytecode->stack_length = MAX(left_cap, right_cap) + 1;
}

void compile_simple_binop(jz_bytecode* bytecode, jz_parse_node* node, jz_opcode op) {
  int left_cap, right_cap;

  compile_node(bytecode, CDAR(node).node);
  left_cap = bytecode->stack_length;

  compile_node(bytecode, CDDR(node).node);
  right_cap = bytecode->stack_length;
  push_opcode(bytecode, op);

  bytecode->stack_length = MAX(left_cap, right_cap + 1);
}

void compile_triop(jz_bytecode* bytecode, jz_parse_node* node) {
  int cap1, cap2, cap3;
  size_t cond_jump, branch1_jump;

  assert(node->car.op_type == jz_op_cond);

  compile_node(bytecode, CDAR(node).node);
  cap1 = bytecode->stack_length;

  push_opcode(bytecode, jz_oc_jump_if);
  cond_jump = push_placeholder(bytecode, JZ_OCS_SIZET);

  compile_node(bytecode, CDDAR(node).node);
  cap2 = bytecode->stack_length;

  push_opcode(bytecode, jz_oc_jump);
  branch1_jump = push_placeholder(bytecode, JZ_OCS_SIZET);
  jump_to_top_from(bytecode, cond_jump);

  compile_node(bytecode, CDDDR(node).node);
  cap3 = bytecode->stack_length;
  jump_to_top_from(bytecode, branch1_jump);

  bytecode->stack_length = MAX(MAX(cap1, cap2), cap3);
}

void compile_literal(jz_bytecode* bytecode, jz_parse_node* node) {
  bytecode->stack_length = 1;
  push_opcode(bytecode, jz_oc_push_literal);
  push_multibyte_arg(bytecode, &(node->car.val), JZ_OCS_TVALUE);
}

void jump_to_top_from(jz_bytecode* bytecode, size_t index) {
  *((size_t*)(bytecode->code + index)) =
    bytecode->code_top - bytecode->code - index - JZ_OCS_SIZET;
}

void push_opcode(jz_bytecode* bytecode, jz_opcode code) {
  if (bytecode->code_top - bytecode->code == bytecode->code_length)
    resize_code(bytecode);
  assert(bytecode->code_top - bytecode->code < bytecode->code_length);

  *(bytecode->code_top) = code;
  bytecode->code_top++;
}

void push_multibyte_arg(jz_bytecode* bytecode, void* data, size_t size) {
  while (bytecode->code_top - bytecode->code + size >= bytecode->code_length)
    resize_code(bytecode);

  memcpy(bytecode->code_top, data, size);
  bytecode->code_top += size;
}

size_t push_placeholder(jz_bytecode* bytecode, size_t size) {
  size_t index = bytecode->code_top - bytecode->code;

  while (index + size >= bytecode->code_length)
    resize_code(bytecode);

  bytecode->code_top += size;
  return index;
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
