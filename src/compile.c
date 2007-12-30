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

#define CAAR(n) ((n)->car.node->car)
#define CADR(n) ((n)->car.node->cdr)
#define CDAR(n) ((n)->cdr.node->car)
#define CDDR(n) ((n)->cdr.node->cdr)
#define CDAAR(n) (CDAR(n).node->car)
#define CDDAR(n) (CDDR(n).node->car)
#define CDDDR(n) (CDDR(n).node->cdr)

#define push_opcode(opcode) jz_opcode_vector_append(state->code, opcode)

static void compile_statements(comp_state* state, jz_parse_node* node);
static void compile_statement(comp_state* state, jz_parse_node* node);
static void compile_vars(comp_state* state, jz_parse_node* node);
static void compile_return(comp_state* state, jz_parse_node* node);
static void compile_if(comp_state* state, jz_parse_node* node);

static void compile_exprs(comp_state* state, jz_parse_node* node);
static void compile_exprs_helper(comp_state* state, jz_parse_node* node, bool first);
static void compile_expr(comp_state* state, jz_parse_node* node);
static lvar_node* compile_identifier(comp_state* state, jz_parse_node* node);
static void compile_literal(comp_state* state, jz_parse_node* node);
static void compile_unop(comp_state* state, jz_parse_node* node);
static void compile_unit_shortcut(comp_state* state, jz_parse_node* node,
                                  jz_opcode op, bool pre);
static void compile_binop(comp_state* state, jz_parse_node* node);
static void compile_logical_binop(comp_state* state, jz_parse_node* node);
static void compile_simple_binop(comp_state* state, jz_parse_node* node, jz_opcode op);
static void compile_assign_binop(comp_state* state, jz_parse_node* node, jz_opcode op);
static void compile_triop(comp_state* state, jz_parse_node* node);

static unsigned char add_lvar(comp_state* state, jz_str* name);
static lvar_node* get_lvar(comp_state* state, jz_str* name);

static void jump_to_top_from(comp_state* state, size_t placeholder);

static void push_multibyte_arg(comp_state* state, void* data, size_t size);
static size_t push_placeholder(comp_state* state, size_t size);

static void free_comp_state(comp_state* state);

JZ_DEFINE_VECTOR(jz_opcode, 20)

jz_bytecode* jz_compile(jz_parse_node* parse_tree) {
  comp_state* state = malloc(sizeof(comp_state));

  state->code = jz_opcode_vector_new();
  state->stack_length = 0;
  state->locals = NULL;

  compile_statements(state, parse_tree);
  push_opcode(jz_oc_end);

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
  compile_statement(state, node->car.node);
  state->stack_length = MAX(old_cap, state->stack_length);
}

static void compile_statement(comp_state* state, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_empty: break;

  case jz_parse_vars:
    compile_vars(state, node);
    break;

  case jz_parse_return:
    compile_return(state, node->car.node);
    break;
      
  case jz_parse_exprs:
    compile_exprs(state, node);
    push_opcode(jz_oc_pop);
    break;

  case jz_parse_if:
    compile_if(state, node);
    break;

  default:
    printf("Unknown statement type %d\n", node->type);
    exit(1);
  }
}

void compile_vars(comp_state* state, jz_parse_node* node) {
  assert(node->type == jz_parse_vars);

  if (node->cdr.node != NULL)
    compile_vars(state, node->cdr.node);

  {
    int old_cap = state->stack_length;
    unsigned char index = add_lvar(state, CAAR(node).str);
    jz_parse_node* expr = CADR(node).node;

    if (expr != NULL)
      compile_expr(state, expr);
    else {
      jz_tvalue undef = jz_undef_val();

      state->stack_length = 1;
      push_opcode(jz_oc_push_literal);
      push_multibyte_arg(state, &undef, JZ_OCS_TVALUE);
    }

    push_opcode(jz_oc_store);
    push_opcode(index);

    state->stack_length = MAX(old_cap, state->stack_length);
  }
}

void compile_return(comp_state* state, jz_parse_node* node) {
  if (node == NULL)
    push_opcode(jz_oc_end);
  else {
    compile_exprs(state, node);
    push_opcode(jz_oc_ret);
  }
}

void compile_if(comp_state* state, jz_parse_node* node) {
  int cap;
  size_t jump;

  compile_exprs(state, node->car.node);
  cap = state->stack_length;

  push_opcode(jz_oc_jump_if);
  jump = push_placeholder(state, JZ_OCS_SIZET);

  compile_statement(state, CDAR(node).node);
  jump_to_top_from(state, jump);

  state->stack_length = MAX(cap, state->stack_length);
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
  if (!first) push_opcode(jz_oc_pop);

  state->stack_length = MAX(old_cap, state->stack_length);
}

void compile_expr(comp_state* state, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_identifier:
    compile_identifier(state, node);
    break;

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

lvar_node* compile_identifier(comp_state* state, jz_parse_node* node) {
  lvar_node* local;

  if (node->type != jz_parse_identifier) {
    fprintf(stderr, "Invalid identifier.\n");
    exit(1);
  }

  local = get_lvar(state, node->car.str);

  if (local == NULL) {
    fprintf(stderr, "Undefined variable %s\n", jz_str_to_chars(node->car.str));
    exit(1);
  }

  state->stack_length = 1;
  push_opcode(jz_oc_retrieve);
  push_opcode(local->index);

  return local;
}

void compile_literal(comp_state* state, jz_parse_node* node) {
  state->stack_length = 1;
  push_opcode(jz_oc_push_literal);
  push_multibyte_arg(state, &(node->car.val), JZ_OCS_TVALUE);
}

#define SIMPLE_UNOP_CASE(operator, opcode)              \
  case operator: {                                      \
    compile_expr(state, node->cdr.node);                \
    push_opcode(opcode);                                \
    break;                                              \
  }

void compile_unop(comp_state* state, jz_parse_node* node) {
  switch (node->car.op_type) {
  SIMPLE_UNOP_CASE(jz_op_add,    jz_oc_to_num)
  SIMPLE_UNOP_CASE(jz_op_sub,    jz_oc_neg)
  SIMPLE_UNOP_CASE(jz_op_bw_not, jz_oc_bw_not)
  SIMPLE_UNOP_CASE(jz_op_not,    jz_oc_not)

  case jz_op_pre_inc:
    compile_unit_shortcut(state, node, jz_oc_add, true);
    break;

  case jz_op_pre_dec:
    compile_unit_shortcut(state, node, jz_oc_sub, true);
    break;

  case jz_op_post_inc:
    compile_unit_shortcut(state, node, jz_oc_add, false);
    break;

  case jz_op_post_dec:
    compile_unit_shortcut(state, node, jz_oc_sub, false);
    break;

  default:
    printf("Unrecognized unary operator %d\n", node->car.op_type);
    exit(1);
  }
}

static void compile_unit_shortcut(comp_state* state, jz_parse_node* node,
                                  jz_opcode op, bool pre) {
  jz_tvalue unit = jz_wrap_num(1);
  lvar_node* var = compile_identifier(state, node->cdr.node);

  if (!pre) push_opcode(jz_oc_dup);
  push_opcode(jz_oc_push_literal);
  push_multibyte_arg(state, &unit, JZ_OCS_TVALUE);
  push_opcode(op);
  if (pre) push_opcode(jz_oc_dup);
  push_opcode(jz_oc_store);
  push_opcode(var->index);

  /* If it's a prefix op, we duplicate the value
     after we increment or decrement it,
     but if it's a postfix, we duplicate first,
     so we need a larger stack size. */
  state->stack_length += pre ? 1 : 2;
}

#define SIMPLE_BINOP_CASE(op)                           \
  case jz_op_ ## op: {                                  \
    compile_simple_binop(state, node, jz_oc_ ## op);    \
    break;                                              \
  }

#define ASSIGN_BINOP_CASE(op)                           \
  case jz_op_ ## op ## _eq: {                           \
    compile_assign_binop(state, node, jz_oc_ ## op);    \
    break;                                              \
  }

void compile_binop(comp_state* state, jz_parse_node* node) {
  switch (node->car.op_type) {
  case jz_op_and:
  case jz_op_or:
    compile_logical_binop(state, node);
    break;

  SIMPLE_BINOP_CASE(bw_or)
  SIMPLE_BINOP_CASE(xor)
  SIMPLE_BINOP_CASE(bw_and)
  SIMPLE_BINOP_CASE(equals)
  SIMPLE_BINOP_CASE(strict_eq)
  SIMPLE_BINOP_CASE(lt)
  SIMPLE_BINOP_CASE(gt)
  SIMPLE_BINOP_CASE(lt_eq)
  SIMPLE_BINOP_CASE(gt_eq)
  SIMPLE_BINOP_CASE(lshift)
  SIMPLE_BINOP_CASE(rshift)
  SIMPLE_BINOP_CASE(urshift)
  SIMPLE_BINOP_CASE(add)
  SIMPLE_BINOP_CASE(sub)
  SIMPLE_BINOP_CASE(times)
  SIMPLE_BINOP_CASE(div)
  SIMPLE_BINOP_CASE(mod)

  case jz_op_assign:
    compile_assign_binop(state, node, jz_oc_noop);
    break;

  ASSIGN_BINOP_CASE(times)
  ASSIGN_BINOP_CASE(div)
  ASSIGN_BINOP_CASE(mod)
  ASSIGN_BINOP_CASE(add)
  ASSIGN_BINOP_CASE(sub)
  ASSIGN_BINOP_CASE(lshift)
  ASSIGN_BINOP_CASE(rshift)
  ASSIGN_BINOP_CASE(urshift)
  ASSIGN_BINOP_CASE(bw_and)
  ASSIGN_BINOP_CASE(xor)
  ASSIGN_BINOP_CASE(bw_or)

  default:
    fprintf(stderr, "Unknown operator %d\n", node->car.op_type);
    exit(1);
  }
}

void compile_logical_binop(comp_state* state, jz_parse_node* node) {
  int left_cap, right_cap;
  size_t jump;

  compile_expr(state, CDAR(node).node);
  left_cap = state->stack_length;
  push_opcode(jz_oc_dup);

  if (node->car.op_type == jz_op_or) push_opcode(jz_oc_not);

  push_opcode(jz_oc_jump_if);
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
  push_opcode(op);

  state->stack_length = MAX(left_cap, right_cap + 1);
}

void compile_assign_binop(comp_state* state, jz_parse_node* node, jz_opcode op) {
  lvar_node* var;

  /* Noop signals that this is just a plain assignment.
     Otherwise we want to run an operation before assigning. */
  if (op != jz_oc_noop) {
    var = compile_identifier(state, CDAR(node).node);
    compile_expr(state, CDDR(node).node);
    push_opcode(op);

    state->stack_length++;
  } else {
    var = get_lvar(state, CDAAR(node).str);
    compile_expr(state, CDDR(node).node);
  }

  push_opcode(jz_oc_dup);
  push_opcode(jz_oc_store);
  push_opcode(var->index);
}

void compile_triop(comp_state* state, jz_parse_node* node) {
  int cap1, cap2, cap3;
  size_t cond_jump, branch1_jump;

  assert(node->car.op_type == jz_op_cond);

  compile_expr(state, CDAR(node).node);
  cap1 = state->stack_length;

  push_opcode(jz_oc_jump_if);
  cond_jump = push_placeholder(state, JZ_OCS_SIZET);

  compile_expr(state, CDDAR(node).node);
  cap2 = state->stack_length;

  push_opcode(jz_oc_jump);
  branch1_jump = push_placeholder(state, JZ_OCS_SIZET);
  jump_to_top_from(state, cond_jump);

  compile_expr(state, CDDDR(node).node);
  cap3 = state->stack_length;
  jump_to_top_from(state, branch1_jump);

  state->stack_length = MAX(MAX(cap1, cap2), cap3);
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

void jump_to_top_from(comp_state* state, size_t placeholder) {
  *((size_t*)(state->code->values + placeholder)) =
    state->code->next - state->code->values - placeholder - JZ_OCS_SIZET;
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
