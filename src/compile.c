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

typedef size_t jz_size_t;
JZ_DECLARE_VECTOR(jz_size_t)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define CAR(n) ((n)->car)
#define CDR(n) ((n)->cdr)
#define CAAR(n) (CAR(CAR(n).node))
#define CADR(n) (CDR(CAR(n).node))
#define CDAR(n) (CAR(CDR(n).node))
#define CDDR(n) (CDR(CDR(n).node))
#define CAAAR(n) (CAR(CAAR(n).node))
#define CDAAR(n) (CAR(CDAR(n).node))
#define CDDAR(n) (CAR(CDDR(n).node))
#define CDDDR(n) (CDR(CDDR(n).node))

#define PUSH_OPCODE(opcode) jz_opcode_vector_append(state->code, opcode)

static void compile_statements(comp_state* state, jz_parse_node* node);
static void compile_statement(comp_state* state, jz_parse_node* node);
static void compile_vars(comp_state* state, jz_parse_node* node);
static void compile_return(comp_state* state, jz_parse_node* node);
static void compile_if(comp_state* state, jz_parse_node* node);
static void compile_do_while(comp_state* state, jz_parse_node* node);
static void compile_while(comp_state* state, jz_parse_node* node);
static void compile_for(comp_state* state, jz_parse_node* node);
static void compile_switch(comp_state* state, jz_parse_node* node);
static jz_size_t_vector* compile_switch_conditionals(comp_state* state, jz_parse_node* node);
static void compile_switch_statements(comp_state* state, jz_parse_node* node, jz_size_t_vector* placeholders);

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

static jz_tvalue* get_literal_value(jz_parse_node* node);

static lvar_node* add_lvar(comp_state* state, jz_str* name, bool* new);
static lvar_node* get_lvar(comp_state* state, jz_str* name);

static void jump_to_top_from(comp_state* state, size_t index);
static void jump_to_from_top(comp_state* state, size_t index);
static void jump_to_from(comp_state* state, size_t to, size_t from);

static void push_multibyte_arg(comp_state* state, void* data, size_t size);
static size_t push_placeholder(comp_state* state, size_t size);

static void free_comp_state(comp_state* state);

JZ_DEFINE_VECTOR(jz_size_t, 10)
JZ_DEFINE_VECTOR(jz_opcode, 20)

jz_bytecode* jz_compile(jz_parse_node* parse_tree) {
  comp_state* state = malloc(sizeof(comp_state));

  state->code = jz_opcode_vector_new();
  state->stack_length = 0;
  state->locals = NULL;

  compile_statements(state, parse_tree);
  PUSH_OPCODE(jz_oc_end);

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

  if (CDR(node).node != NULL)
    compile_statements(state, CDR(node).node);

  old_cap = state->stack_length;
  compile_statement(state, CAR(node).node);
  state->stack_length = MAX(old_cap, state->stack_length);
}

static void compile_statement(comp_state* state, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_empty: break;

  case jz_parse_statements:
    compile_statements(state, node);
    break;

  case jz_parse_vars:
    compile_vars(state, node);
    break;

  case jz_parse_return:
    compile_return(state, CAR(node).node);
    break;
      
  case jz_parse_exprs:
    compile_exprs(state, node);
    PUSH_OPCODE(jz_oc_pop);
    break;

  case jz_parse_if:
    compile_if(state, node);
    break;

  case jz_parse_do_while:
    compile_do_while(state, node);
    break;

  case jz_parse_while:
    compile_while(state, node);
    break;

  case jz_parse_for:
    compile_for(state, node);
    break;

  case jz_parse_switch:
    compile_switch(state, node);
    break;

  default:
    printf("Unknown statement type %d\n", node->type);
    exit(1);
  }
}

void compile_vars(comp_state* state, jz_parse_node* node) {
  assert(node->type == jz_parse_vars);

  if (CDR(node).node != NULL)
    compile_vars(state, CDR(node).node);

  node = CAR(node).node;
  assert(node->type == jz_parse_var);

  {
    int old_cap = state->stack_length;
    bool new_node;
    unsigned char index = add_lvar(state, CAR(node).str, &new_node)->index;
    jz_parse_node* expr = CDR(node).node;

    if (expr != NULL) {
      compile_expr(state, expr);

      PUSH_OPCODE(jz_oc_store);
      PUSH_OPCODE(index);
    } else if (new_node) {
      jz_tvalue undef = jz_undef_val();

      state->stack_length = 1;
      PUSH_OPCODE(jz_oc_push_literal);
      push_multibyte_arg(state, &undef, JZ_OCS_TVALUE);

      PUSH_OPCODE(jz_oc_store);
      PUSH_OPCODE(index);
    } else state->stack_length = 0;

    state->stack_length = MAX(old_cap, state->stack_length);
  }
}

void compile_return(comp_state* state, jz_parse_node* node) {
  if (node == NULL)
    PUSH_OPCODE(jz_oc_end);
  else {
    compile_exprs(state, node);
    PUSH_OPCODE(jz_oc_ret);
  }
}

void compile_if(comp_state* state, jz_parse_node* node) {
  int expr_cap, if_cap;
  size_t jump;

  compile_exprs(state, CAR(node).node);
  expr_cap = state->stack_length;

  PUSH_OPCODE(jz_oc_jump_unless);
  jump = push_placeholder(state, JZ_OCS_SIZET);

  compile_statement(state, CDAR(node).node);
  jump_to_top_from(state, jump);

  if (CDDR(node).node != NULL) {
    compile_statement(state, CDDR(node).node);
    if_cap = state->stack_length;
  } else if_cap = 0;

  state->stack_length = MAX(MAX(expr_cap, if_cap), state->stack_length);
}

void compile_do_while(comp_state* state, jz_parse_node* node) {
  int cap;
  size_t jump;
  jz_tvalue* conditional_literal = get_literal_value(CAR(node).node);

  /* If the conditional is a literal value that evaluates to true,
     we don't bother to check it. */
  bool skip_conditional = conditional_literal != NULL &&
    jz_to_bool(*conditional_literal);

  jump = state->code->next - state->code->values;
  compile_statement(state, CDR(node).node);
  cap = state->stack_length;

  if (skip_conditional)
    PUSH_OPCODE(jz_oc_jump);
  else {
    compile_exprs(state, CAR(node).node);
    PUSH_OPCODE(jz_oc_jump_if);
  }

  jump_to_from_top(state, jump);

  state->stack_length = MAX(cap, state->stack_length);
}

void compile_while(comp_state* state, jz_parse_node* node) {
  int cap;
  size_t index, placeholder;
  jz_tvalue* conditional_literal = get_literal_value(CAR(node).node);

  /* If the conditional is NULL or a literal value that evaluates to true,
     we don't bother to check it.
     This makes stuff like "for (;;)" and "while (true)" faster. */
  bool skip_conditional = CAR(node).node == NULL ||
    (conditional_literal != NULL && jz_to_bool(*conditional_literal));

  index = state->code->next - state->code->values;

  if (skip_conditional) cap = 0;
  else {
    compile_exprs(state, CAR(node).node);
    cap = state->stack_length;

    PUSH_OPCODE(jz_oc_jump_unless);
    placeholder = push_placeholder(state, JZ_OCS_SIZET);
  }

  compile_statement(state, CDR(node).node);
  PUSH_OPCODE(jz_oc_jump);
  jump_to_from_top(state, index);

  if (!skip_conditional) jump_to_top_from(state, placeholder);

  state->stack_length = MAX(cap, state->stack_length);
}

void compile_for(comp_state* state, jz_parse_node* node) {
  int cap;
  jz_parse_node *body, *inc_expr, *while_statement;

  if (CAR(node).node != NULL) {
    compile_statement(state, CAR(node).node);
    cap = state->stack_length;
  } else cap = 0;

  inc_expr = CDDAR(node).node;
  if (inc_expr != NULL) {
    inc_expr = jz_pnode_wrap(jz_parse_exprs, inc_expr);
    body = jz_pnode_cons(jz_parse_statements, inc_expr,
                         jz_pnode_wrap(jz_parse_statements, CDDDR(node).node));
  } else body = CDDDR(node).node;

  while_statement = jz_pnode_cons(jz_parse_while, CDAR(node).node, body);
  compile_statement(state, while_statement);

  state->stack_length = MAX(cap, state->stack_length);
}

void compile_switch(comp_state* state, jz_parse_node* node) {
  jz_size_t_vector* placeholders;
  int expr_cap, cond_cap;

  compile_exprs(state, CAR(node).node);
  expr_cap = state->stack_length;

  placeholders = compile_switch_conditionals(state, CDR(node).node);
  cond_cap = state->stack_length;

  compile_switch_statements(state, CDR(node).node, placeholders);
  state->stack_length = MAX(expr_cap, MAX(cond_cap, state->stack_length)) + 1;

  PUSH_OPCODE(jz_oc_pop);

  jz_size_t_vector_free(placeholders);
}

jz_size_t_vector* compile_switch_conditionals(comp_state* state, jz_parse_node* node) {
  jz_size_t_vector* placeholders = jz_size_t_vector_new();

  if (node == NULL) return placeholders;

  while (node != NULL) {
    jz_parse_node* case_node = CAR(node).node;
    int old_cap = state->stack_length;

    node = CDR(node).node;

    if (CAR(case_node).node == NULL) continue;

    PUSH_OPCODE(jz_oc_dup);
    compile_exprs(state, CAR(case_node).node);
    PUSH_OPCODE(jz_oc_strict_eq);
    PUSH_OPCODE(jz_oc_jump_if);
    jz_size_t_vector_append(placeholders,
                            push_placeholder(state, JZ_OCS_SIZET));

    state->stack_length = MAX(old_cap, state->stack_length + 1);
  }

  PUSH_OPCODE(jz_oc_jump);
  jz_size_t_vector_append(placeholders,
                          push_placeholder(state, JZ_OCS_SIZET));

  return placeholders;
}

void compile_switch_statements(comp_state* state, jz_parse_node* node, jz_size_t_vector* placeholders) {
  size_t* next_placeholder = placeholders->values;
  size_t default_pos = -1; /* -1 indicates that there is no default case. */

  if (node == NULL) return;

  while (node != NULL) {
    jz_parse_node* case_node = CAR(node).node;

    /* If this is the default case,
       we only want to jump to it after trying all other conditionals. */
    if (CAR(case_node).node == NULL)
      default_pos = state->code->next - state->code->values;
    else {
      jump_to_top_from(state, *(next_placeholder++));
    }

    if (CDR(case_node).node != NULL) {
      int old_cap = state->stack_length;
      compile_statements(state, CDR(case_node).node);
      state->stack_length = MAX(old_cap, state->stack_length);
    }

    node = CDR(node).node;
  }

  if (default_pos != -1)
    jump_to_from(state, default_pos, *next_placeholder);
  else
    jump_to_top_from(state, *next_placeholder);
}

void compile_exprs(comp_state* state, jz_parse_node* node) {
  compile_exprs_helper(state, node, true);
}

void compile_exprs_helper(comp_state* state, jz_parse_node* node, bool first) {
  int old_cap;

  assert(node->type == jz_parse_exprs);

  if (CDR(node).node != NULL) {
    compile_exprs_helper(state, CDR(node).node, false);
    old_cap = state->stack_length;
  } else old_cap = 0;

  compile_expr(state, CAR(node).node);

  /* Discard the return value of all expressions in a list but the last. */
  if (!first) PUSH_OPCODE(jz_oc_pop);

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

  local = get_lvar(state, CAR(node).str);

  if (local == NULL) {
    fprintf(stderr, "Undefined variable %s\n", jz_str_to_chars(CAR(node).str));
    exit(1);
  }

  state->stack_length = 1;
  PUSH_OPCODE(jz_oc_retrieve);
  PUSH_OPCODE(local->index);

  return local;
}

void compile_literal(comp_state* state, jz_parse_node* node) {
  state->stack_length = 1;
  PUSH_OPCODE(jz_oc_push_literal);
  push_multibyte_arg(state, CAR(node).val, JZ_OCS_TVALUE);
}

#define SIMPLE_UNOP_CASE(operator, opcode)              \
  case operator: {                                      \
    compile_expr(state, CDR(node).node);                \
    PUSH_OPCODE(opcode);                                \
    break;                                              \
  }

void compile_unop(comp_state* state, jz_parse_node* node) {
  switch (*CAR(node).op_type) {
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
    printf("Unrecognized unary operator %d\n", *CAR(node).op_type);
    exit(1);
  }
}

static void compile_unit_shortcut(comp_state* state, jz_parse_node* node,
                                  jz_opcode op, bool pre) {
  jz_tvalue unit = jz_wrap_num(1);
  lvar_node* var = compile_identifier(state, CDR(node).node);

  if (!pre) PUSH_OPCODE(jz_oc_dup);
  PUSH_OPCODE(jz_oc_push_literal);
  push_multibyte_arg(state, &unit, JZ_OCS_TVALUE);
  PUSH_OPCODE(op);
  if (pre) PUSH_OPCODE(jz_oc_dup);
  PUSH_OPCODE(jz_oc_store);
  PUSH_OPCODE(var->index);

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
  switch (*CAR(node).op_type) {
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
    fprintf(stderr, "Unknown operator %d\n", *CAR(node).op_type);
    exit(1);
  }
}

void compile_logical_binop(comp_state* state, jz_parse_node* node) {
  int left_cap, right_cap;
  size_t jump;

  compile_expr(state, CDAR(node).node);
  left_cap = state->stack_length;
  PUSH_OPCODE(jz_oc_dup);

  PUSH_OPCODE(*CAR(node).op_type == jz_op_or ? jz_oc_jump_if : jz_oc_jump_unless);
  jump = push_placeholder(state, JZ_OCS_SIZET);

  PUSH_OPCODE(jz_oc_pop);
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
  PUSH_OPCODE(op);

  state->stack_length = MAX(left_cap, right_cap + 1);
}

void compile_assign_binop(comp_state* state, jz_parse_node* node, jz_opcode op) {
  lvar_node* var;

  /* Noop signals that this is just a plain assignment.
     Otherwise we want to run an operation before assigning. */
  if (op != jz_oc_noop) {
    var = compile_identifier(state, CDAR(node).node);
    compile_expr(state, CDDR(node).node);
    PUSH_OPCODE(op);
  } else {
    var = get_lvar(state, CDAAR(node).str);
    compile_expr(state, CDDR(node).node);
  }

  state->stack_length++;
  PUSH_OPCODE(jz_oc_dup);
  PUSH_OPCODE(jz_oc_store);
  PUSH_OPCODE(var->index);
}

void compile_triop(comp_state* state, jz_parse_node* node) {
  int cap1, cap2, cap3;
  size_t cond_jump, branch1_jump;

  assert(*CAR(node).op_type == jz_op_cond);

  compile_expr(state, CDAR(node).node);
  cap1 = state->stack_length;

  PUSH_OPCODE(jz_oc_jump_unless);
  cond_jump = push_placeholder(state, JZ_OCS_SIZET);

  compile_expr(state, CDDAR(node).node);
  cap2 = state->stack_length;

  PUSH_OPCODE(jz_oc_jump);
  branch1_jump = push_placeholder(state, JZ_OCS_SIZET);
  jump_to_top_from(state, cond_jump);

  compile_expr(state, CDDDR(node).node);
  cap3 = state->stack_length;
  jump_to_top_from(state, branch1_jump);

  state->stack_length = MAX(MAX(cap1, cap2), cap3);
}

/* Get the jz_tvalue of a jz_parse_exprs node if it's just a literal value,
   or NULL if it's not. */
jz_tvalue* get_literal_value(jz_parse_node* node) {
  if (node == NULL ||
      node->type != jz_parse_exprs ||
      CDR(node).node != NULL ||
      CAR(node).node->type != jz_parse_literal)
    return NULL;
  return CAAR(node).val;
}

lvar_node* add_lvar(comp_state* state, jz_str* name, bool* new) {
  lvar_node* node;

  if ((node = get_lvar(state, name))) {
    *new = false;
    return node;
  } else *new = true;

  node = malloc(sizeof(lvar_node));
  node->next = state->locals;
  node->name = name;
  node->index = state->locals == NULL ? 0 : state->locals->index + 1;
  state->locals = node;

  return node;
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
  jump_to_from(state, state->code->next - state->code->values, index);
}

void jump_to_from_top(comp_state* state, size_t index) {
  index = index - JZ_OCS_SIZET - (state->code->next - state->code->values);
  push_multibyte_arg(state, &index, JZ_OCS_SIZET);
}

void jump_to_from(comp_state* state, size_t to, size_t from) {
  *((size_t*)(state->code->values + from)) = to - from - JZ_OCS_SIZET;
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

void jz_free_bytecode(jz_bytecode* this) {
  if (this == NULL) return;

  free(this->code);
  free(this);
}

