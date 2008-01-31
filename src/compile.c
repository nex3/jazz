#include "compile.h"
#include "string.h"

#include <stdio.h>

typedef struct lvar_node lvar_node;
struct lvar_node {
  lvar_node* next;
  jz_str* name;
  jz_index index;
};

typedef struct const_node const_node;
struct const_node {
  const_node* next;
  jz_tvalue val;
};

typedef struct {
  jz_opcode_vector* code;
  size_t stack_length;
  lvar_node* locals;
  const_node* consts;
  size_t consts_length;
} comp_state;

#define STATE JZ_STATE, comp_state* state

typedef ptrdiff_t jz_ptrdiff;
JZ_DECLARE_VECTOR(jz_ptrdiff)

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

#define PUSH_OPCODE(opcode) jz_opcode_vector_append(jz, state->code, opcode)
#define PUSH_ARG(arg) \
  push_multibyte_arg(jz, state, &(arg), sizeof(arg)/sizeof(jz_opcode))

#define IS_INDEX_OP(node) \
  ((node)->type == jz_parse_binop && *CAR(node).op_type == jz_op_index)

static jz_tvalue* consts_to_array(STATE);

static void compile_statements(STATE, jz_parse_node* node);
static void compile_statement(STATE, jz_parse_node* node);
static void compile_vars(STATE, jz_parse_node* node);
static void compile_var(STATE, jz_parse_node* node);
static void compile_return(STATE, jz_parse_node* node);
static void compile_if(STATE, jz_parse_node* node);
static void compile_do_while(STATE, jz_parse_node* node);
static void compile_while(STATE, jz_parse_node* node);
static void compile_for(STATE, jz_parse_node* node);
static void compile_switch(STATE, jz_parse_node* node);
static jz_ptrdiff_vector* compile_switch_conditionals(STATE, jz_parse_node* node);
static void compile_switch_statements(STATE, jz_parse_node* node, jz_ptrdiff_vector* placeholders);

static void compile_exprs(STATE, jz_parse_node* node, bool value);
static void compile_expr(STATE, jz_parse_node* node, bool value);
static lvar_node* compile_identifier(STATE, jz_parse_node* node, bool value);
static void compile_literal(STATE, jz_parse_node* node, bool value);
static void compile_unop(STATE, jz_parse_node* node, bool value);
static void compile_unit_shortcut(STATE, jz_parse_node* node,
                                  jz_opcode op, bool pre, bool value);
static void compile_binop(STATE, jz_parse_node* node, bool value);
static void compile_logical_binop(STATE, jz_parse_node* node, bool value);
static void compile_simple_binop(STATE, jz_parse_node* node, jz_opcode op, bool value);
static void compile_triop(STATE, jz_parse_node* node, bool value);
static void compile_assign_binop(STATE, jz_parse_node* node, jz_opcode op, bool value);
static void compile_identifier_assign(STATE, jz_parse_node* node, jz_opcode op, bool value);
static void compile_index_assign(STATE, jz_parse_node* node, jz_opcode op, bool value);

static jz_tvalue* get_literal_value(jz_parse_node* node);

static lvar_node* add_lvar(STATE, jz_str* name, bool* new);
static lvar_node* get_lvar(STATE, jz_str* name);

static jz_index add_const(STATE, jz_tvalue value);

static void jump_to_top_from(STATE, ptrdiff_t index);
static void jump_to_from_top(STATE, ptrdiff_t index);
static void jump_to_from(STATE, ptrdiff_t to, ptrdiff_t from);

static void push_multibyte_arg(STATE, const void* data, size_t size);
static ptrdiff_t push_placeholder(STATE, size_t size);

static void free_comp_state(STATE);

static void free_list(JZ_STATE, jz_parse_node* head);

JZ_DEFINE_VECTOR(jz_ptrdiff, 10)
JZ_DEFINE_VECTOR(jz_opcode, 20)

jz_bytecode* jz_compile(JZ_STATE, jz_parse_node* parse_tree) {
  comp_state* state = malloc(sizeof(comp_state));

  state->code = jz_opcode_vector_new(jz);
  state->stack_length = 0;
  state->locals = NULL;
  state->consts = NULL;
  state->consts_length = 0;

  compile_statements(jz, state, parse_tree);
  PUSH_OPCODE(jz_oc_end);

  {
    jz_bytecode* bytecode = malloc(sizeof(jz_bytecode));

    bytecode->stack_length = state->stack_length;
    bytecode->locals_length =
      state->locals == NULL ? 0 : state->locals->index + 1;
    bytecode->code_length = state->code->next - state->code->values;
    bytecode->code = calloc(sizeof(jz_opcode), bytecode->code_length);
    memcpy(bytecode->code, state->code->values, bytecode->code_length);
    bytecode->consts = consts_to_array(jz, state);
    bytecode->consts_length = state->consts_length;

    free_comp_state(jz, state);
    return bytecode;
  }
}

jz_tvalue* consts_to_array(STATE) {
  const_node* node;
  jz_tvalue* bottom = calloc(sizeof(jz_tvalue), state->consts_length);
  jz_tvalue* top = bottom;

  for (node = state-> consts; node != NULL; node = node->next)
    *top++ = node->val;

  return bottom;
}

void compile_statements(STATE, jz_parse_node* node) {
  while (node != NULL) {
    int old_cap;

    assert(node->type == jz_parse_statements);

    old_cap = state->stack_length;
    compile_statement(jz, state, CAR(node).node);
    state->stack_length = MAX(old_cap, state->stack_length);

    node = CDR(node).node;
  }
}

static void compile_statement(STATE, jz_parse_node* node) {
  switch (node->type) {
  case jz_parse_empty: break;

  case jz_parse_statements:
    compile_statements(jz, state, node);
    break;

  case jz_parse_vars:
    compile_vars(jz, state, node);
    break;

  case jz_parse_return:
    compile_return(jz, state, CAR(node).node);
    break;
      
  case jz_parse_exprs:
    compile_exprs(jz, state, node, false);
    break;

  case jz_parse_if:
    compile_if(jz, state, node);
    break;

  case jz_parse_do_while:
    compile_do_while(jz, state, node);
    break;

  case jz_parse_while:
    compile_while(jz, state, node);
    break;

  case jz_parse_for:
    compile_for(jz, state, node);
    break;

  case jz_parse_switch:
    compile_switch(jz, state, node);
    break;

  default:
    printf("Unknown statement type %d\n", node->type);
    exit(1);
  }
}

void compile_vars(STATE, jz_parse_node* node) {
  while (node != NULL) {
    assert(node->type == jz_parse_vars);

    compile_var(jz, state, CAR(node).node);
    node = CDR(node).node;
  }
}

void compile_var(STATE, jz_parse_node* node) {
  int old_cap;
  bool new_node;
  jz_index index;
  jz_parse_node* expr;

  assert(node->type == jz_parse_var);

  old_cap = state->stack_length;
  index = add_lvar(jz, state, CAR(node).str, &new_node)->index;
  expr = CDR(node).node;

  if (expr != NULL) {
    compile_expr(jz, state, expr, true);

    PUSH_OPCODE(jz_oc_store);
    PUSH_ARG(index);
  } else state->stack_length = 0;

  state->stack_length = MAX(old_cap, state->stack_length);
}

void compile_return(STATE, jz_parse_node* node) {
  if (node == NULL)
    PUSH_OPCODE(jz_oc_end);
  else {
    compile_exprs(jz, state, node, true);
    PUSH_OPCODE(jz_oc_ret);
  }
}

void compile_if(STATE, jz_parse_node* node) {
  int expr_cap, if_cap;
  ptrdiff_t jump;

  compile_exprs(jz, state, CAR(node).node, true);
  expr_cap = state->stack_length;

  PUSH_OPCODE(jz_oc_jump_unless);
  jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

  compile_statement(jz, state, CDAR(node).node);

  if (CDDR(node).node == NULL) {
    jump_to_top_from(jz, state, jump);
    if_cap = 0;
  } else {
    ptrdiff_t else_jump;

    /* We'll want to jump past the else clause if it exists. */
    PUSH_OPCODE(jz_oc_jump);
    else_jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

    jump_to_top_from(jz, state, jump); /* Jump past the new jump instruction
                                      if the if statement fails. */


    compile_statement(jz, state, CDDR(node).node);
    if_cap = state->stack_length;

    jump_to_top_from(jz, state, else_jump);
  }

  state->stack_length = MAX(MAX(expr_cap, if_cap), state->stack_length);
}

void compile_do_while(STATE, jz_parse_node* node) {
  int cap;
  ptrdiff_t jump;
  jz_tvalue* conditional_literal = get_literal_value(CAR(node).node);

  /* If the conditional is a literal value that evaluates to true,
     we don't bother to check it. */
  bool skip_conditional = conditional_literal != NULL &&
    jz_to_bool(jz, *conditional_literal);

  jump = state->code->next - state->code->values;
  compile_statement(jz, state, CDR(node).node);
  cap = state->stack_length;

  if (skip_conditional)
    PUSH_OPCODE(jz_oc_jump);
  else {
    compile_exprs(jz, state, CAR(node).node, true);
    PUSH_OPCODE(jz_oc_jump_if);
  }

  jump_to_from_top(jz, state, jump);

  state->stack_length = MAX(cap, state->stack_length);
}

void compile_while(STATE, jz_parse_node* node) {
  int cap;
  ptrdiff_t index, placeholder;
  jz_tvalue* conditional_literal = get_literal_value(CAR(node).node);

  /* If the conditional is NULL or a literal value that evaluates to true,
     we don't bother to check it.
     This makes stuff like "for (;;)" and "while (true)" faster. */
  bool skip_conditional = CAR(node).node == NULL ||
    (conditional_literal != NULL && jz_to_bool(jz, *conditional_literal));

  index = state->code->next - state->code->values;

  if (skip_conditional) cap = 0;
  else {
    compile_exprs(jz, state, CAR(node).node, true);
    cap = state->stack_length;

    PUSH_OPCODE(jz_oc_jump_unless);
    placeholder = push_placeholder(jz, state, JZ_OCS_PTRDIFF);
  }

  compile_statement(jz, state, CDR(node).node);
  PUSH_OPCODE(jz_oc_jump);
  jump_to_from_top(jz, state, index);

  if (!skip_conditional) jump_to_top_from(jz, state, placeholder);

  state->stack_length = MAX(cap, state->stack_length);
}

void compile_for(STATE, jz_parse_node* node) {
  int cap;
  jz_parse_node *body, *inc_expr, *while_statement;

  if (CAR(node).node != NULL) {
    compile_statement(jz, state, CAR(node).node);
    cap = state->stack_length;
  } else cap = 0;

  inc_expr = CDDAR(node).node;
  if (inc_expr != NULL) {
    inc_expr = jz_pnode_wrap(jz, jz_parse_exprs, inc_expr);
    body = jz_pnode_cons(jz, jz_parse_statements, inc_expr,
                         jz_pnode_wrap(jz, jz_parse_statements, CDDDR(node).node));
  } else body = CDDDR(node).node;

  while_statement = jz_pnode_cons(jz, jz_parse_while, CDAR(node).node, body);
  compile_statement(jz, state, while_statement);

  state->stack_length = MAX(cap, state->stack_length);

  /* Now free the new nodes we've made. */
  if (inc_expr != NULL) {
    free(CAR(body).node);
    free(CDR(body).node);
    free(body);
  }

  free(while_statement);
}

void compile_switch(STATE, jz_parse_node* node) {
  jz_ptrdiff_vector* placeholders;
  int expr_cap, cond_cap;

  compile_exprs(jz, state, CAR(node).node, true);
  expr_cap = state->stack_length;

  placeholders = compile_switch_conditionals(jz, state, CDR(node).node);
  cond_cap = state->stack_length;

  compile_switch_statements(jz, state, CDR(node).node, placeholders);
  state->stack_length = MAX(expr_cap, MAX(cond_cap, state->stack_length)) + 1;

  PUSH_OPCODE(jz_oc_pop);

  jz_ptrdiff_vector_free(jz, placeholders);
}

jz_ptrdiff_vector* compile_switch_conditionals(STATE, jz_parse_node* node) {
  jz_ptrdiff_vector* placeholders = jz_ptrdiff_vector_new(jz);

  if (node == NULL) return placeholders;

  while (node != NULL) {
    jz_parse_node* case_node = CAR(node).node;
    int old_cap = state->stack_length;

    node = CDR(node).node;

    if (CAR(case_node).node == NULL) continue;

    PUSH_OPCODE(jz_oc_dup);
    compile_exprs(jz, state, CAR(case_node).node, true);
    PUSH_OPCODE(jz_oc_strict_eq);
    PUSH_OPCODE(jz_oc_jump_if);
    jz_ptrdiff_vector_append(jz, placeholders,
                             push_placeholder(jz, state, JZ_OCS_PTRDIFF));

    state->stack_length = MAX(old_cap, state->stack_length + 1);
  }

  PUSH_OPCODE(jz_oc_jump);
  jz_ptrdiff_vector_append(jz, placeholders,
                           push_placeholder(jz, state, JZ_OCS_PTRDIFF));

  return placeholders;
}

void compile_switch_statements(STATE, jz_parse_node* node, jz_ptrdiff_vector* placeholders) {
  ptrdiff_t* next_placeholder = placeholders->values;
  ptrdiff_t default_pos = -1; /* -1 indicates that there is no default case. */

  if (node == NULL) return;

  while (node != NULL) {
    jz_parse_node* case_node = CAR(node).node;

    /* If this is the default case,
       we only want to jump to it after trying all other conditionals. */
    if (CAR(case_node).node == NULL)
      default_pos = state->code->next - state->code->values;
    else {
      jump_to_top_from(jz, state, *(next_placeholder++));
    }

    if (CDR(case_node).node != NULL) {
      int old_cap = state->stack_length;
      compile_statements(jz, state, CDR(case_node).node);
      state->stack_length = MAX(old_cap, state->stack_length);
    }

    node = CDR(node).node;
  }

  if (default_pos != -1)
    jump_to_from(jz, state, default_pos, *next_placeholder);
  else
    jump_to_top_from(jz, state, *next_placeholder);
}

void compile_exprs(STATE, jz_parse_node* node, bool value) {
  bool first = true;

  while (node != NULL) {
    int old_cap = first ? 0 : state->stack_length;
    bool this_value = false;

    /* Discard the return value of all expressions in a list
       except maybe the last. */
    if (CDR(node).node == NULL)
      this_value = value;

    assert(node->type == jz_parse_exprs);

    compile_expr(jz, state, CAR(node).node, this_value);

    state->stack_length = MAX(old_cap, state->stack_length);

    first = false;
    node = CDR(node).node;
  }
}

void compile_expr(STATE, jz_parse_node* node, bool value) {
  switch (node->type) {
  case jz_parse_identifier:
    compile_identifier(jz, state, node, value);
    break;

  case jz_parse_literal:
    compile_literal(jz, state, node, value);
    break;

  case jz_parse_exprs:
    compile_exprs(jz, state, node, value);
    break;

  case jz_parse_unop:
    compile_unop(jz, state, node, value);
    break;

  case jz_parse_binop:
    compile_binop(jz, state, node, value);
    break;

  case jz_parse_triop:
    compile_triop(jz, state, node, value);
    break;

  default:
    printf("Unrecognized expression node type %d\n", node->type);
    exit(1);
  }
}

lvar_node* compile_identifier(STATE, jz_parse_node* node, bool value) {
  lvar_node* local;

  if (node->type != jz_parse_identifier) {
    fprintf(stderr, "Invalid identifier.\n");
    exit(1);
  }

  local = get_lvar(jz, state, CAR(node).str);

  if (local == NULL) {
    fprintf(stderr, "Undefined variable %s\n", jz_str_to_chars(jz, CAR(node).str));
    exit(1);
  }

  if (!value) {
    state->stack_length = 1;
    return NULL;
  }

  state->stack_length = 1;
  PUSH_OPCODE(jz_oc_retrieve);
  PUSH_ARG(local->index);

  return local;
}

void compile_literal(STATE, jz_parse_node* node, bool value) {
  jz_index index = add_const(jz, state, *CAR(node).val);

  if (!value) {
    state->stack_length = 0;
    return;
  }

  state->stack_length = 1;
  PUSH_OPCODE(jz_oc_push_literal);
  PUSH_ARG(index);
}

#define SIMPLE_UNOP_CASE(operator, opcode)              \
  case operator: {                                      \
    compile_expr(jz, state, CDR(node).node, true);      \
    PUSH_OPCODE(opcode);                                \
                                                        \
    if (!value)                                         \
      PUSH_OPCODE(jz_oc_pop);                           \
                                                        \
    break;                                              \
  }

void compile_unop(STATE, jz_parse_node* node, bool value) {
  switch (*CAR(node).op_type) {
  SIMPLE_UNOP_CASE(jz_op_add,    jz_oc_to_num)
  SIMPLE_UNOP_CASE(jz_op_sub,    jz_oc_neg)
  SIMPLE_UNOP_CASE(jz_op_bw_not, jz_oc_bw_not)
  SIMPLE_UNOP_CASE(jz_op_not,    jz_oc_not)

  case jz_op_pre_inc:
    compile_unit_shortcut(jz, state, node, jz_oc_add, true, value);
    break;

  case jz_op_pre_dec:
    compile_unit_shortcut(jz, state, node, jz_oc_sub, true, value);
    break;

  case jz_op_post_inc:
    compile_unit_shortcut(jz, state, node, jz_oc_add, false, value);
    break;

  case jz_op_post_dec:
    compile_unit_shortcut(jz, state, node, jz_oc_sub, false, value);
    break;

  default:
    printf("Unrecognized unary operator %d\n", *CAR(node).op_type);
    exit(1);
  }
}

static void compile_unit_shortcut(STATE, jz_parse_node* node,
                                  jz_opcode op, bool pre, bool value) {
  jz_index unit_index = add_const(jz, state, jz_wrap_num(jz, 1));
  lvar_node* var = compile_identifier(jz, state, CDR(node).node, true);
  assert(var != NULL);

  if (!pre && value)
    PUSH_OPCODE(jz_oc_dup);

  PUSH_OPCODE(jz_oc_push_literal);
  PUSH_ARG(unit_index);
  PUSH_OPCODE(op);

  if (pre && value)
    PUSH_OPCODE(jz_oc_dup);

  PUSH_OPCODE(jz_oc_store);
  PUSH_ARG(var->index);

  /* If it's a prefix op, we duplicate the value
     after we increment or decrement it,
     but if it's a postfix, we duplicate first,
     so we need a larger stack size. */
  state->stack_length += pre ? 1 : 2;
}

#define SIMPLE_BINOP_CASE(op)                           \
  case jz_op_ ## op: {                                  \
    compile_simple_binop(jz, state, node, jz_oc_ ## op, value); \
    break;                                              \
  }

#define ASSIGN_BINOP_CASE(op)                           \
  case jz_op_ ## op ## _eq: {                           \
    compile_assign_binop(jz, state, node, jz_oc_ ## op, value); \
    break;                                              \
  }

void compile_binop(STATE, jz_parse_node* node, bool value) {
  switch (*CAR(node).op_type) {
  case jz_op_and:
  case jz_op_or:
    compile_logical_binop(jz, state, node, value);
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
    compile_assign_binop(jz, state, node, jz_oc_noop, value);
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

  SIMPLE_BINOP_CASE(index)

  default:
    fprintf(stderr, "Unknown operator %d\n", *CAR(node).op_type);
    exit(1);
  }
}

void compile_logical_binop(STATE, jz_parse_node* node, bool value) {
  int left_cap, right_cap;
  ptrdiff_t jump;

  compile_expr(jz, state, CDAR(node).node, true);
  left_cap = state->stack_length;

  if (value)
    PUSH_OPCODE(jz_oc_dup);

  PUSH_OPCODE(*CAR(node).op_type == jz_op_or ? jz_oc_jump_if : jz_oc_jump_unless);
  jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

  if (value)
    PUSH_OPCODE(jz_oc_pop);

  compile_expr(jz, state, CDDR(node).node, value);
  right_cap = state->stack_length;
  jump_to_top_from(jz, state, jump);

  state->stack_length = MAX(left_cap, right_cap) + 1;
}

void compile_simple_binop(STATE, jz_parse_node* node, jz_opcode op, bool value) {
  int left_cap, right_cap;

  compile_expr(jz, state, CDAR(node).node, true);
  left_cap = state->stack_length;

  compile_expr(jz, state, CDDR(node).node, true);
  right_cap = state->stack_length;

  if (op != jz_oc_noop)
    PUSH_OPCODE(op);
  else if (!value)
    PUSH_OPCODE(jz_oc_pop);

  state->stack_length = MAX(left_cap, right_cap + 1);
}

void compile_triop(STATE, jz_parse_node* node, bool value) {
  int cap1, cap2, cap3;
  ptrdiff_t cond_jump, branch1_jump;

  assert(*CAR(node).op_type == jz_op_cond);

  compile_expr(jz, state, CDAR(node).node, true);
  cap1 = state->stack_length;

  PUSH_OPCODE(jz_oc_jump_unless);
  cond_jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

  compile_expr(jz, state, CDDAR(node).node, value);
  cap2 = state->stack_length;

  PUSH_OPCODE(jz_oc_jump);
  branch1_jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);
  jump_to_top_from(jz, state, cond_jump);

  compile_expr(jz, state, CDDDR(node).node, value);
  cap3 = state->stack_length;
  jump_to_top_from(jz, state, branch1_jump);

  state->stack_length = MAX(MAX(cap1, cap2), cap3);
}

void compile_assign_binop(STATE, jz_parse_node* node, jz_opcode op, bool value) {
  jz_parse_node* left = CDAR(node).node;

  if (left->type == jz_parse_identifier)
    compile_identifier_assign(jz, state, node, op, value);
  else if (IS_INDEX_OP(left))
    compile_index_assign(jz, state, node, op, value);
  else {
    fprintf(stderr, "Invalid left-hand side of assignment.\n");
    exit(1);
  }
}

void compile_identifier_assign(STATE, jz_parse_node* node, jz_opcode op, bool value) {
  jz_parse_node* left = CDAR(node).node;
  jz_parse_node* right = CDDR(node).node;
  lvar_node* var;

  /* Noop signals that this is just a plain assignment.
     Otherwise we want to run an operation before assigning. */
  if (op == jz_oc_noop) {
    var = get_lvar(jz, state, CAR(left).str);
    compile_expr(jz, state, right, true);
  } else {
    var = compile_identifier(jz, state, left, true);

    assert(var != NULL);
    compile_expr(jz, state, right, true);
    PUSH_OPCODE(op);
  }

  if (var == NULL) {
    fprintf(stderr, "Undefined identifier \"%s\"\n",
            jz_str_to_chars(jz, CAR(left).str));
    exit(1);
  }

  state->stack_length++;

  if (value)
    PUSH_OPCODE(jz_oc_dup);

  PUSH_OPCODE(jz_oc_store);
  PUSH_ARG(var->index);
}

void compile_index_assign(STATE, jz_parse_node* node, jz_opcode op, bool value) {
  jz_parse_node* left = CDAR(node).node;
  jz_parse_node* right = CDDR(node).node;
  int left_cap, right_cap;
  char base_stack_size = 2;

  compile_simple_binop(jz, state, left, jz_oc_noop, true);
  left_cap = state->stack_length;

  /* Noop signals that this is just a plain assignment.
     Otherwise we want to run an operation before assigning. */
  if (op != jz_oc_noop) {
    PUSH_OPCODE(jz_oc_dup2);
    PUSH_OPCODE(jz_oc_index);
    base_stack_size++;
  }

  compile_expr(jz, state, right, true);
  right_cap = state->stack_length;

  if (op != jz_oc_noop)
    PUSH_OPCODE(op);

  if (value) {
    PUSH_OPCODE(jz_oc_dup);
    PUSH_OPCODE(jz_oc_rot4);
  }

  PUSH_OPCODE(jz_oc_index_store);

  state->stack_length = MAX(left_cap, right_cap + base_stack_size) +
    (value || op != jz_oc_noop ? 1 : 0);
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

lvar_node* add_lvar(STATE, jz_str* name, bool* new) {
  lvar_node* node;

  if ((node = get_lvar(jz, state, name))) {
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

lvar_node* get_lvar(STATE, jz_str* name) {
  lvar_node* node = state->locals;

  while (node != NULL) {
    if (jz_str_equal(jz, name, node->name)) return node;
    node = node->next;
  }

  return NULL;
}

jz_index add_const(STATE, jz_tvalue value) {
  const_node* last_node = NULL;
  const_node* node = state->consts;
  jz_index index = 0;

  while (node != NULL) {
    if (jz_values_strict_equal(jz, value, node->val)) return index;
    last_node = node;
    node = node->next;
    index++;
  }

  node = malloc(sizeof(const_node));
  node->next = NULL;
  node->val = value;
  state->consts_length++;

  if (last_node == NULL) state->consts = node;
  else last_node->next = node;

  return index;
}

void jump_to_top_from(STATE, ptrdiff_t index) {
  jump_to_from(jz, state, state->code->next - state->code->values, index);
}

void jump_to_from_top(STATE, ptrdiff_t index) {
  index = index - JZ_OCS_PTRDIFF - (state->code->next - state->code->values);
  PUSH_ARG(index);
}

void jump_to_from(STATE, ptrdiff_t to, ptrdiff_t from) {
  *((ptrdiff_t*)(state->code->values + from)) = to - from - JZ_OCS_PTRDIFF;
}

void push_multibyte_arg(STATE, const void* data, size_t size) {
  jz_opcode_vector* vector = state->code;

  while (vector->next - vector->values + size >= vector->capacity)
    jz_opcode_vector_resize(jz, vector);

  memcpy(vector->next, data, size);
  vector->next += size;
}

ptrdiff_t push_placeholder(STATE, size_t size) {
  jz_opcode_vector* vector = state->code;
  ptrdiff_t index = vector->next - vector->values;

  while (index + size >= vector->capacity)
    jz_opcode_vector_resize(jz, vector);

  vector->next += size;
  return index;
}

void free_comp_state(STATE) {
  while (state->locals != NULL) {
    lvar_node* old_locals = state->locals;
    state->locals = state->locals->next;
    free(old_locals);
  }

  while (state->consts != NULL) {
    const_node* old_consts = state->consts;
    state->consts = state->consts->next;
    free(old_consts);
  }

  jz_opcode_vector_free(jz, state->code);
  free(state);
}

void jz_free_bytecode(JZ_STATE, jz_bytecode* this) {
  if (this == NULL) return;

  free(this->code);
  free(this->consts);
  free(this);
}


void jz_free_parse_tree(JZ_STATE, jz_parse_node* root) {
  if (root == NULL) return;

  switch (root->type) {
  case jz_parse_cont:
    fprintf(stderr, "Error: manually freeing continuation node\n");
    exit(1);
  case jz_parse_var:
    jz_free_parse_tree(jz, CDR(root).node);
    break;
  case jz_parse_literal:
    free(CAR(root).val);
    break;
  case jz_parse_unop:
    free(CAR(root).op_type);
    jz_free_parse_tree(jz, CDR(root).node);
    break;
  case jz_parse_binop:
    free(CAR(root).op_type);
    jz_free_parse_tree(jz, CDAR(root).node);
    jz_free_parse_tree(jz, CDDR(root).node);
    free(CDR(root).node);
    break;
  case jz_parse_triop:
    free(CAR(root).op_type);
    jz_free_parse_tree(jz, CDAR(root).node);
    jz_free_parse_tree(jz, CDDAR(root).node);
    jz_free_parse_tree(jz, CDDDR(root).node);
    free(CDDR(root).node);
    free(CDR(root).node);
    break;
  case jz_parse_statements:
  case jz_parse_cases:
  case jz_parse_vars:
  case jz_parse_exprs:
    free_list(jz, root);
    return;
  case jz_parse_return:
    jz_free_parse_tree(jz, CAR(root).node);
    break;
  case jz_parse_do_while:
  case jz_parse_while:
  case jz_parse_switch:
  case jz_parse_case:
    jz_free_parse_tree(jz, CAR(root).node);
    jz_free_parse_tree(jz, CDR(root).node);
    break;
  case jz_parse_if:
    jz_free_parse_tree(jz, CAR(root).node);
    jz_free_parse_tree(jz, CDAR(root).node);
    jz_free_parse_tree(jz, CDDR(root).node);
    free(CDR(root).node);
    break;
  case jz_parse_for:
    jz_free_parse_tree(jz, CAR(root).node);
    jz_free_parse_tree(jz, CDAR(root).node);
    jz_free_parse_tree(jz, CDDAR(root).node);
    jz_free_parse_tree(jz, CDDDR(root).node);
    free(CDDR(root).node);
    free(CDR(root).node);
    break;
  case jz_parse_identifier:
  case jz_parse_empty: break;
  default:
    fprintf(stderr, "Unrecognized node type %d\n", root->type);
    exit(1);
  }

  free(root);
}

void free_list(JZ_STATE, jz_parse_node* head) {
  while (head != NULL) {
    jz_parse_node* next = CDR(head).node;

    jz_free_parse_tree(jz, CAR(head).node);
    free(head);
    head = next;
  }
}
