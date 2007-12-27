#include "compile.h"

#include <stdio.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define CAAR(node) ((node)->car.node->car)
#define CADR(node) ((node)->car.node->cdr)
#define CDAR(node) ((node)->cdr.node->car)
#define CDDR(node) ((node)->cdr.node->cdr)
#define CDDAR(node) (CDDR(node).node->car)
#define CDDDR(node) (CDDR(node).node->cdr)

static void compile_statements(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_return(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_exprs(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_exprs_helper(jz_bytecode* bytecode, jz_parse_node* node, bool first);
static void compile_expr(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_unop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_binop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_logical_binop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_simple_binop(jz_bytecode* bytecode, jz_parse_node* node, jz_opcode op);
static void compile_triop(jz_bytecode* bytecode, jz_parse_node* node);
static void compile_literal(jz_bytecode* bytecode, jz_parse_node* node);

static void jump_to_top_from(jz_bytecode* bytecode, size_t index);

static void push_multibyte_arg(jz_bytecode* bytecode, void* data, size_t size);
static size_t push_placeholder(jz_bytecode* bytecode, size_t size);

JZ_DEFINE_VECTOR(jz_opcode, 20)

jz_bytecode* jz_compile(jz_parse_node* parse_tree) {
  jz_bytecode* bytecode = malloc(sizeof(jz_bytecode));

  bytecode->code = jz_opcode_vector_new();
  bytecode->stack_length = 0;

  compile_statements(bytecode, parse_tree);
  jz_opcode_vector_append(bytecode->code, jz_oc_end);

  return bytecode;
}

void compile_statements(jz_bytecode* bytecode, jz_parse_node* node) {
  int old_cap;

  assert(node->type == jz_parse_statements);

  if (node->cdr.node != NULL)
    compile_statements(bytecode, node->cdr.node);

  old_cap = bytecode->stack_length;
  node = node->car.node;
  assert(node->type == jz_parse_statement);
    
  switch (node->car.st_type) {
  case jz_st_empty: break;

  case jz_st_return:
    compile_return(bytecode, node->cdr.node);
    break;
      
  case jz_st_expr:
    compile_exprs(bytecode, node->cdr.node);
    jz_opcode_vector_append(bytecode->code, jz_oc_pop);
    break;

  default:
    printf("Unknown statement type %d\n", node->car.st_type);
    exit(1);
  }

  bytecode->stack_length = MAX(old_cap, bytecode->stack_length);
}

void compile_return(jz_bytecode* bytecode, jz_parse_node* node) {
  if (node == NULL)
    jz_opcode_vector_append(bytecode->code, jz_oc_end);
  else {
    compile_exprs(bytecode, node);
    jz_opcode_vector_append(bytecode->code, jz_oc_ret);
  }
}

void compile_exprs(jz_bytecode* bytecode, jz_parse_node* node) {
  compile_exprs_helper(bytecode, node, true);
}

void compile_exprs_helper(jz_bytecode* bytecode, jz_parse_node* node, bool first) {
  int old_cap;

  assert(node->type == jz_parse_exprs);

  if (node->cdr.node != NULL)
    compile_exprs_helper(bytecode, node->cdr.node, false);

  old_cap = bytecode->stack_length;
  compile_expr(bytecode, node->car.node);

  /* Discard the return value of all expressions in a list but the last. */
  if (!first)
    jz_opcode_vector_append(bytecode->code, jz_oc_pop);

  bytecode->stack_length = MAX(old_cap, bytecode->stack_length);
}

void compile_expr(jz_bytecode* bytecode, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_literal:
    compile_literal(bytecode, node);
    break;

  case jz_parse_exprs:
    compile_exprs(bytecode, node);
    break;

  case jz_parse_unop:
    compile_unop(bytecode, node);
    break;

  case jz_parse_binop:
    compile_binop(bytecode, node);
    break;

  case jz_parse_triop:
    compile_triop(bytecode, node);
    break;

  default:
    printf("Unrecognized expression node type %d\n", node->type);
    exit(1);
  }
}

void compile_unop(jz_bytecode* bytecode, jz_parse_node* node) {
  compile_expr(bytecode, node->cdr.node);

  switch (node->car.op_type) {
  case jz_op_plus:
    jz_opcode_vector_append(bytecode->code, jz_oc_to_num);
    break;

  case jz_op_minus:
    jz_opcode_vector_append(bytecode->code, jz_oc_neg);
    break;

  case jz_op_bw_not:
    jz_opcode_vector_append(bytecode->code, jz_oc_bw_not);
    break;

  case jz_op_not:
    jz_opcode_vector_append(bytecode->code, jz_oc_not);
    break;

  default:
    printf("Unrecognized unary operator %d\n", node->car.op_type);
    exit(1);
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

  case jz_op_equals:
    compile_simple_binop(bytecode, node, jz_oc_equal);
    break;

  case jz_op_eq_eq_eq:
    compile_simple_binop(bytecode, node, jz_oc_strict_equal);
    break;

  case jz_op_lt:
    compile_simple_binop(bytecode, node, jz_oc_lt);
    break;

  case jz_op_gt:
    compile_simple_binop(bytecode, node, jz_oc_gt);
    break;

  case jz_op_lt_eq:
    compile_simple_binop(bytecode, node, jz_oc_lte);
    break;

  case jz_op_gt_eq:
    compile_simple_binop(bytecode, node, jz_oc_gte);
    break;

  case jz_op_lt_lt:
    compile_simple_binop(bytecode, node, jz_oc_lshift);
    break;

  case jz_op_gt_gt:
    compile_simple_binop(bytecode, node, jz_oc_rshift);
    break;

  case jz_op_gt_gt_gt:
    compile_simple_binop(bytecode, node, jz_oc_urshift);
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

  case jz_op_mod:
    compile_simple_binop(bytecode, node, jz_oc_mod);
    break;

  default: assert(0);
  }
}

void compile_logical_binop(jz_bytecode* bytecode, jz_parse_node* node) {
  int left_cap, right_cap;
  size_t jump;

  compile_expr(bytecode, CDAR(node).node);
  left_cap = bytecode->stack_length;
  jz_opcode_vector_append(bytecode->code, jz_oc_dup);

  if (node->car.op_type == jz_op_or) jz_opcode_vector_append(bytecode->code, jz_oc_not);

  jz_opcode_vector_append(bytecode->code, jz_oc_jump_if);
  jump = push_placeholder(bytecode, JZ_OCS_SIZET);

  compile_expr(bytecode, CDDR(node).node);
  right_cap = bytecode->stack_length;
  jump_to_top_from(bytecode, jump);

  bytecode->stack_length = MAX(left_cap, right_cap) + 1;
}

void compile_simple_binop(jz_bytecode* bytecode, jz_parse_node* node, jz_opcode op) {
  int left_cap, right_cap;

  compile_expr(bytecode, CDAR(node).node);
  left_cap = bytecode->stack_length;

  compile_expr(bytecode, CDDR(node).node);
  right_cap = bytecode->stack_length;
  jz_opcode_vector_append(bytecode->code, op);

  bytecode->stack_length = MAX(left_cap, right_cap + 1);
}

void compile_triop(jz_bytecode* bytecode, jz_parse_node* node) {
  int cap1, cap2, cap3;
  size_t cond_jump, branch1_jump;

  assert(node->car.op_type == jz_op_cond);

  compile_expr(bytecode, CDAR(node).node);
  cap1 = bytecode->stack_length;

  jz_opcode_vector_append(bytecode->code, jz_oc_jump_if);
  cond_jump = push_placeholder(bytecode, JZ_OCS_SIZET);

  compile_expr(bytecode, CDDAR(node).node);
  cap2 = bytecode->stack_length;

  jz_opcode_vector_append(bytecode->code, jz_oc_jump);
  branch1_jump = push_placeholder(bytecode, JZ_OCS_SIZET);
  jump_to_top_from(bytecode, cond_jump);

  compile_expr(bytecode, CDDDR(node).node);
  cap3 = bytecode->stack_length;
  jump_to_top_from(bytecode, branch1_jump);

  bytecode->stack_length = MAX(MAX(cap1, cap2), cap3);
}

void compile_literal(jz_bytecode* bytecode, jz_parse_node* node) {
  bytecode->stack_length = 1;
  jz_opcode_vector_append(bytecode->code, jz_oc_push_literal);
  push_multibyte_arg(bytecode, &(node->car.val), JZ_OCS_TVALUE);
}

void jump_to_top_from(jz_bytecode* bytecode, size_t index) {
  *((size_t*)(bytecode->code->values + index)) =
    bytecode->code->next - bytecode->code->values - index - JZ_OCS_SIZET;
}

void push_multibyte_arg(jz_bytecode* bytecode, void* data, size_t size) {
  jz_opcode_vector* vector = bytecode->code;

  while (vector->next - vector->values + size >= vector->capacity)
    jz_opcode_vector_resize(vector);

  memcpy(vector->next, data, size);
  vector->next += size;
}

size_t push_placeholder(jz_bytecode* bytecode, size_t size) {
  jz_opcode_vector* vector = bytecode->code;
  size_t index = vector->next - vector->values;

  while (index + size >= vector->capacity)
    jz_opcode_vector_resize(vector);

  vector->next += size;
  return index;
}
