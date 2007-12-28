#include "compile.h"

#include <stdio.h>

typedef struct lvar_node lvar_node;

struct lvar_node {
  lvar_node* next;
  jz_str* name;
  unsigned char index;
};

typedef struct {
  jz_opcode_vector* code;
  size_t stack_length;
  lvar_node* locals;
} comp_state;

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define CAAR(node) ((node)->car.node->car)
#define CADR(node) ((node)->car.node->cdr)
#define CDAR(node) ((node)->cdr.node->car)
#define CDDR(node) ((node)->cdr.node->cdr)
#define CDDAR(node) (CDDR(node).node->car)
#define CDDDR(node) (CDDR(node).node->cdr)

static void compile_statements(comp_state* state, jz_parse_node* node);
static void compile_vars(comp_state* state, jz_parse_node* node);
static void compile_return(comp_state* state, jz_parse_node* node);

static void compile_exprs(comp_state* state, jz_parse_node* node);
static void compile_exprs_helper(comp_state* state, jz_parse_node* node, bool first);
static void compile_expr(comp_state* state, jz_parse_node* node);
static void compile_unop(comp_state* state, jz_parse_node* node);
static void compile_binop(comp_state* state, jz_parse_node* node);
static void compile_logical_binop(comp_state* state, jz_parse_node* node);
static void compile_simple_binop(comp_state* state, jz_parse_node* node, jz_opcode op);
static void compile_triop(comp_state* state, jz_parse_node* node);
static void compile_literal(comp_state* state, jz_parse_node* node);

static unsigned char add_lvar(comp_state* state, jz_str* name);
static lvar_node* get_lvar(comp_state* state, jz_str* name);

static void jump_to_top_from(comp_state* state, size_t index);

static void push_multibyte_arg(comp_state* state, void* data, size_t size);
static size_t push_placeholder(comp_state* state, size_t size);

static void free_comp_state(comp_state* state);

JZ_DEFINE_VECTOR(jz_opcode, 20)

jz_bytecode* jz_compile(jz_parse_node* parse_tree) {
  comp_state* state = malloc(sizeof(comp_state));

  state->code = jz_opcode_vector_new();
  state->stack_length = 0;

  compile_statements(state, parse_tree);
  jz_opcode_vector_append(state->code, jz_oc_end);

  {
    jz_bytecode* bytecode = malloc(sizeof(jz_bytecode));
    bytecode->stack_length = state->stack_length;
    bytecode->locals_length =
      state->locals == NULL ? 0 : state->locals->index + 1;
    bytecode->code_length = state->code->next - state->code->values;
    bytecode->code = calloc(sizeof(jz_opcode), bytecode->code_length);
    memcpy(bytecode->code, state->code->values, bytecode->code_length);

    free_comp_state(state);
    return bytecode;
  }
}

void compile_statements(comp_state* state, jz_parse_node* node) {
  int old_cap;

  assert(node->type == jz_parse_statements);

  if (node->cdr.node != NULL)
    compile_statements(state, node->cdr.node);

  old_cap = state->stack_length;
  node = node->car.node;
  assert(node->type == jz_parse_statement);
    
  switch (node->car.st_type) {
  case jz_st_empty: break;

  case jz_st_var:
    compile_vars(state, node->cdr.node);
    break;

  case jz_st_return:
    compile_return(state, node->cdr.node);
    break;
      
  case jz_st_expr:
    compile_exprs(state, node->cdr.node);
    jz_opcode_vector_append(state->code, jz_oc_pop);
    break;

  default:
    printf("Unknown statement type %d\n", node->car.st_type);
    exit(1);
  }

  state->stack_length = MAX(old_cap, state->stack_length);
}

void compile_vars(comp_state* state, jz_parse_node* node) {
  assert(node->type == jz_parse_vars);

  if (node->cdr.node != NULL)
    compile_vars(state, node->cdr.node);

  {
    int old_cap = state->stack_length;
    unsigned char index = add_lvar(state, CAAR(node).str);
    jz_parse_node* expr = CADR(node).node;

    if (expr == NULL) return;

    compile_expr(state, expr);
    jz_opcode_vector_append(state->code, jz_oc_store);
    jz_opcode_vector_append(state->code, index);

    state->stack_length = MAX(old_cap, state->stack_length);
  }
}

void compile_return(comp_state* state, jz_parse_node* node) {
  if (node == NULL)
    jz_opcode_vector_append(state->code, jz_oc_end);
  else {
    compile_exprs(state, node);
    jz_opcode_vector_append(state->code, jz_oc_ret);
  }
}

void compile_exprs(comp_state* state, jz_parse_node* node) {
  compile_exprs_helper(state, node, true);
}

void compile_exprs_helper(comp_state* state, jz_parse_node* node, bool first) {
  int old_cap;

  assert(node->type == jz_parse_exprs);

  if (node->cdr.node != NULL)
    compile_exprs_helper(state, node->cdr.node, false);

  old_cap = state->stack_length;
  compile_expr(state, node->car.node);

  /* Discard the return value of all expressions in a list but the last. */
  if (!first)
    jz_opcode_vector_append(state->code, jz_oc_pop);

  state->stack_length = MAX(old_cap, state->stack_length);
}

void compile_expr(comp_state* state, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_literal:
    compile_literal(state, node);
    break;

  case jz_parse_exprs:
    compile_exprs(state, node);
    break;

  case jz_parse_unop:
    compile_unop(state, node);
    break;

  case jz_parse_binop:
    compile_binop(state, node);
    break;

  case jz_parse_triop:
    compile_triop(state, node);
    break;

  default:
    printf("Unrecognized expression node type %d\n", node->type);
    exit(1);
  }
}

void compile_unop(comp_state* state, jz_parse_node* node) {
  compile_expr(state, node->cdr.node);

  switch (node->car.op_type) {
  case jz_op_plus:
    jz_opcode_vector_append(state->code, jz_oc_to_num);
    break;

  case jz_op_minus:
    jz_opcode_vector_append(state->code, jz_oc_neg);
    break;

  case jz_op_bw_not:
    jz_opcode_vector_append(state->code, jz_oc_bw_not);
    break;

  case jz_op_not:
    jz_opcode_vector_append(state->code, jz_oc_not);
    break;

  default:
    printf("Unrecognized unary operator %d\n", node->car.op_type);
    exit(1);
  }
}

void compile_binop(comp_state* state, jz_parse_node* node) {
  switch (node->car.op_type) {
  case jz_op_and:
  case jz_op_or:
    compile_logical_binop(state, node);
    break;

  case jz_op_bw_or:
    compile_simple_binop(state, node, jz_oc_bw_or);
    break;

  case jz_op_xor:
    compile_simple_binop(state, node, jz_oc_xor);
    break;

  case jz_op_bw_and:
    compile_simple_binop(state, node, jz_oc_bw_and);
    break;

  case jz_op_equals:
    compile_simple_binop(state, node, jz_oc_equal);
    break;

  case jz_op_eq_eq_eq:
    compile_simple_binop(state, node, jz_oc_strict_equal);
    break;

  case jz_op_lt:
    compile_simple_binop(state, node, jz_oc_lt);
    break;

  case jz_op_gt:
    compile_simple_binop(state, node, jz_oc_gt);
    break;

  case jz_op_lt_eq:
    compile_simple_binop(state, node, jz_oc_lte);
    break;

  case jz_op_gt_eq:
    compile_simple_binop(state, node, jz_oc_gte);
    break;

  case jz_op_lt_lt:
    compile_simple_binop(state, node, jz_oc_lshift);
    break;

  case jz_op_gt_gt:
    compile_simple_binop(state, node, jz_oc_rshift);
    break;

  case jz_op_gt_gt_gt:
    compile_simple_binop(state, node, jz_oc_urshift);
    break;

  case jz_op_plus:
    compile_simple_binop(state, node, jz_oc_add);
    break;

  case jz_op_minus:
    compile_simple_binop(state, node, jz_oc_sub);
    break;

  case jz_op_times:
    compile_simple_binop(state, node, jz_oc_times);
    break;

  case jz_op_div:
    compile_simple_binop(state, node, jz_oc_div);
    break;

  case jz_op_mod:
    compile_simple_binop(state, node, jz_oc_mod);
    break;

  default: assert(0);
  }
}

void compile_logical_binop(comp_state* state, jz_parse_node* node) {
  int left_cap, right_cap;
  size_t jump;

  compile_expr(state, CDAR(node).node);
  left_cap = state->stack_length;
  jz_opcode_vector_append(state->code, jz_oc_dup);

  if (node->car.op_type == jz_op_or) jz_opcode_vector_append(state->code, jz_oc_not);

  jz_opcode_vector_append(state->code, jz_oc_jump_if);
  jump = push_placeholder(state, JZ_OCS_SIZET);

  compile_expr(state, CDDR(node).node);
  right_cap = state->stack_length;
  jump_to_top_from(state, jump);

  state->stack_length = MAX(left_cap, right_cap) + 1;
}

void compile_simple_binop(comp_state* state, jz_parse_node* node, jz_opcode op) {
  int left_cap, right_cap;

  compile_expr(state, CDAR(node).node);
  left_cap = state->stack_length;

  compile_expr(state, CDDR(node).node);
  right_cap = state->stack_length;
  jz_opcode_vector_append(state->code, op);

  state->stack_length = MAX(left_cap, right_cap + 1);
}

void compile_triop(comp_state* state, jz_parse_node* node) {
  int cap1, cap2, cap3;
  size_t cond_jump, branch1_jump;

  assert(node->car.op_type == jz_op_cond);

  compile_expr(state, CDAR(node).node);
  cap1 = state->stack_length;

  jz_opcode_vector_append(state->code, jz_oc_jump_if);
  cond_jump = push_placeholder(state, JZ_OCS_SIZET);

  compile_expr(state, CDDAR(node).node);
  cap2 = state->stack_length;

  jz_opcode_vector_append(state->code, jz_oc_jump);
  branch1_jump = push_placeholder(state, JZ_OCS_SIZET);
  jump_to_top_from(state, cond_jump);

  compile_expr(state, CDDDR(node).node);
  cap3 = state->stack_length;
  jump_to_top_from(state, branch1_jump);

  state->stack_length = MAX(MAX(cap1, cap2), cap3);
}

void compile_literal(comp_state* state, jz_parse_node* node) {
  state->stack_length = 1;
  jz_opcode_vector_append(state->code, jz_oc_push_literal);
  push_multibyte_arg(state, &(node->car.val), JZ_OCS_TVALUE);
}

unsigned char add_lvar(comp_state* state, jz_str* name) {
  lvar_node* node;

  if ((node = get_lvar(state, name))) return node->index;

  node = malloc(sizeof(lvar_node));
  node->next = state->locals;
  node->name = name;
  node->index = state->locals == NULL ? 0 : state->locals->index + 1;
  state->locals = node;

  return node->index;
}

lvar_node* get_lvar(comp_state* state, jz_str* name) {
  lvar_node* node = state->locals;

  while (node != NULL) {
    if (jz_str_equal(name, node->name)) return node;
    node = node->next;
  }

  return NULL;
}

void jump_to_top_from(comp_state* state, size_t index) {
  *((size_t*)(state->code->values + index)) =
    state->code->next - state->code->values - index - JZ_OCS_SIZET;
}

void push_multibyte_arg(comp_state* state, void* data, size_t size) {
  jz_opcode_vector* vector = state->code;

  while (vector->next - vector->values + size >= vector->capacity)
    jz_opcode_vector_resize(vector);

  memcpy(vector->next, data, size);
  vector->next += size;
}

size_t push_placeholder(comp_state* state, size_t size) {
  jz_opcode_vector* vector = state->code;
  size_t index = vector->next - vector->values;

  while (index + size >= vector->capacity)
    jz_opcode_vector_resize(vector);

  vector->next += size;
  return index;
}

void free_comp_state(comp_state* state) {
  while (state->locals != NULL) {
    lvar_node* old_locals = state->locals;
    state->locals = state->locals->next;
    free(old_locals);
  }

  jz_opcode_vector_free(state->code);
  free(state);
}
