#include "compile.h"
#include "string.h"
#include "function.h"
#include "state.h"
#include "_cons.h"
#include "traverse.h"
#include "object.h"

#include <stdio.h>

typedef struct {
  enum {
    local_var,
    closure_var,
    global_var
  } type;
  jz_str* name;
  jz_index index;
} variable;

typedef struct const_node const_node;
struct const_node {
  const_node* next;
  jz_val val;
};

typedef struct comp_state comp_state;
struct comp_state {
  comp_state* scope;
  jz_opcode_vector* code;
  jz_obj* local_vars;
  jz_obj* closure_vars;
  size_t closure_vars_length;
  const_node* consts;
  size_t consts_length;
};

#define STATE JZ_STATE, comp_state* state

typedef ptrdiff_t jz_ptrdiff;
JZ_DECLARE_VECTOR(jz_ptrdiff)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PUSH_OPCODE(opcode) jz_opcode_vector_append(jz, state->code, opcode)
#define PUSH_ARG(arg) \
  push_multibyte_arg(jz, state, &(arg), sizeof(arg)/sizeof(jz_opcode))


static comp_state* comp_state_new(JZ_STATE, comp_state* scope);
static jz_bytecode* compile(STATE, jz_cons* parse_tree);

static jz_val* consts_to_array(STATE);

static jz_bool init_funcs(JZ_STATE, jz_cons* node, void* data);
static jz_bool analyze_vars(JZ_STATE, jz_cons* node, void* data);
static jz_bool analyze_identifiers(JZ_STATE, jz_cons* node, void* data);
static jz_bool assign_indices(STATE);
static void assign_indices_iterator(JZ_STATE, jz_str* key, jz_val val, void* data);
static jz_bool assign_indices_walker(JZ_STATE, jz_cons* node, void* data);

static void compile_statements(STATE, jz_cons* node);
static void compile_statement(STATE, jz_cons* node);
static void compile_vars(STATE, jz_cons* node);
static void compile_var(STATE, jz_cons* node);
static void compile_return(STATE, jz_cons* node);
static void compile_if(STATE, jz_cons* node);
static void compile_do_while(STATE, jz_cons* node);
static void compile_while(STATE, jz_cons* node);
static void compile_for(STATE, jz_cons* node);
static void compile_switch(STATE, jz_cons* node);
static jz_ptrdiff_vector* compile_switch_conditionals(STATE, jz_cons* node);
static void compile_switch_statements(STATE, jz_cons* node, jz_ptrdiff_vector* placeholders);

static void compile_expr(STATE, jz_cons* node, jz_bool value);
static variable* compile_identifier(STATE, jz_cons* node, jz_bool value);
static void compile_identifier_store(STATE, variable* var);
static void compile_literal(STATE, jz_cons* node, jz_bool value);
static void compile_this(STATE, jz_cons* node, jz_bool value);
static void compile_unop(STATE, jz_cons* node, jz_parse_type op, jz_bool value);
static void compile_unit_shortcut(STATE, jz_cons* node,
                                  jz_opcode op, jz_bool pre, jz_bool value);
static void compile_binop(STATE, jz_cons* node, jz_parse_type op, jz_bool value);
static void compile_comma(STATE, jz_cons* node, jz_bool value);
static void compile_logical_binop(STATE, jz_cons* node, jz_parse_type op, jz_bool value);
static void compile_simple_binop(STATE, jz_cons* node, jz_opcode op, jz_bool value);
static void compile_cond(STATE, jz_cons* node, jz_bool value);
static void compile_call(STATE, jz_cons* node, jz_bool value);
static void compile_func(STATE, jz_cons* node, jz_bool value);
static void compile_assign_binop(STATE, jz_cons* node, jz_opcode op, jz_bool value);
static void compile_identifier_assign(STATE, jz_cons* node, jz_opcode op, jz_bool value);
static void compile_index_assign(STATE, jz_cons* node, jz_opcode op, jz_bool value);

static jz_val get_literal_value(JZ_STATE, jz_cons* node, jz_bool* success);

static variable* get_var(STATE, jz_str* name, jz_bool from_inner_scope);
static variable* add_lvar(STATE, jz_str* name);

static jz_index add_const(STATE, jz_val value);

static void jump_to_top_from(STATE, ptrdiff_t index);
static void jump_to_from_top(STATE, ptrdiff_t index);
static void jump_to_from(STATE, ptrdiff_t to, ptrdiff_t from);

static void push_multibyte_arg(STATE, const void* data, size_t size);
static ptrdiff_t push_placeholder(STATE, size_t size);

static void free_comp_state(STATE);
static void free_variable(JZ_STATE, jz_str* key, jz_val val, void* data);

JZ_DEFINE_VECTOR(jz_ptrdiff, 10)
JZ_DEFINE_VECTOR(jz_opcode, 20)

jz_bytecode* jz_compile(JZ_STATE, jz_cons* parse_tree) {
  comp_state* state = comp_state_new(jz, NULL);

  jz_traverse_several(jz, parse_tree, analyze_vars, state,
                      2, jz_parse_var, jz_parse_func);
  jz_traverse_one(jz, parse_tree, init_funcs, state, jz_parse_func);
  jz_traverse_several(jz, parse_tree, analyze_identifiers, state,
                      3, jz_parse_identifier, jz_parse_var, jz_parse_func);

  assign_indices(jz, state);
  jz_traverse_one(jz, parse_tree, assign_indices_walker, state, jz_parse_func);

  return compile(jz, state, parse_tree);
}

comp_state* comp_state_new(JZ_STATE, comp_state* scope) {
  comp_state* state = malloc(sizeof(comp_state));

  state->scope  = scope;
  state->code   = jz_opcode_vector_new(jz);
  state->local_vars = jz_obj_new_bare(jz);
  state->closure_vars = jz_obj_new_bare(jz);
  state->closure_vars_length = 0;
  state->consts = NULL;
  state->consts_length = 0;

  return state;
}

jz_bytecode* compile(STATE, jz_cons* parse_tree) {
  jz_traverse_several(jz, parse_tree, analyze_vars, state,
                      2, jz_parse_var, jz_parse_func);

  compile_statements(jz, state, parse_tree);
  PUSH_OPCODE(jz_oc_end);

  {
    jz_bytecode* bytecode = malloc(sizeof(jz_bytecode));

    bytecode->locals_length = state->local_vars->size;
    bytecode->closure_vars_length = state->closure_vars_length;
    bytecode->closure_locals_length = state->closure_vars->size;
    bytecode->code_length = state->code->next - state->code->values;
    bytecode->code = calloc(sizeof(jz_opcode), bytecode->code_length);
    memcpy(bytecode->code, state->code->values, bytecode->code_length);
    bytecode->consts = consts_to_array(jz, state);
    bytecode->consts_length = state->consts_length;

    free_comp_state(jz, state);
    return bytecode;
  }
}

jz_val* consts_to_array(STATE) {
  const_node* node;
  jz_val* bottom = calloc(sizeof(jz_val), state->consts_length);
  jz_val* top = bottom;

  for (node = state-> consts; node != NULL; node = node->next)
    *top++ = node->val;

  return bottom;
}

/* Sets cadr.ptr of each func to be that func's comp_state. */
jz_bool init_funcs(JZ_STATE, jz_cons* node, void* data) {
  comp_state* scope = (comp_state*)data;
  comp_state* state = comp_state_new(jz, scope);

  /* First we figure out what our local vars are... */
  jz_traverse_several(jz, NODE(CDR(node)), analyze_vars, state,
                      2, jz_parse_var, jz_parse_func);

  /* Then we init sub-funcs, thus figuring out what our closure vars are... */
  jz_traverse_one(jz, NODE(CDR(node)), init_funcs, state, jz_parse_func);

  /* And finally we record where our own vars point. */
  jz_traverse_several(jz, NODE(CDR(node)), analyze_identifiers, state,
                      3, jz_parse_identifier, jz_parse_var, jz_parse_func);

  CDR(node) = CONS(VOID(state), CDR(node));

  return jz_false;
}

jz_bool analyze_vars(JZ_STATE, jz_cons* node, void* data) {
  comp_state* state = (comp_state*)data;
  jz_cons* next = NODE(CDR(node));

  if (ENUM(CAR(node)) == jz_parse_func)
    return jz_false;

  while (next != NULL) {
    ASSERT_GC_TYPE(CAAR(next), jz_t_str);
    add_lvar(jz, state, (jz_str*)CAAR(next));
    next = NODE(CDR(next));
  }

  return jz_true;
}

jz_bool analyze_identifiers(JZ_STATE, jz_cons* node, void* data) {
  comp_state* state = (comp_state*)data;
  variable* var;

  if (ENUM(CAR(node)) == jz_parse_func)
    return jz_false;

  if (ENUM(CAR(node)) == jz_parse_var) {
    jz_cons* var_node = NODE(CDR(node));

    while (var_node != NULL) {
      ASSERT_GC_TYPE(CAAR(var_node), jz_t_str);
      var = get_var(jz, state, (jz_str*)CAAR(var_node), jz_false);
      CDAR(var_node) = CONS(VOID(var), CDAR(var_node));

      var_node = NODE(CDR(var_node));
    }

    return jz_true;
  }

  assert(ENUM(CAR(node)) == jz_parse_identifier);
  ASSERT_GC_TYPE(CADR(node), jz_t_str);
  var = get_var(jz, state, (jz_str*)CADR(node), jz_false);
  CDR(node) = CONS(VOID(var), CDR(node));

  return jz_true;
}

jz_bool assign_indices(STATE) {
  int i = state->scope == NULL ? 0 : state->scope->closure_vars_length;
  jz_obj_each(jz, state->closure_vars, assign_indices_iterator, &i);
  state->closure_vars_length = i;

  return jz_true;
}

void assign_indices_iterator(JZ_STATE, jz_str* key, jz_val val, void* data) {
  int* i = (int*)data;
  variable* var;

  assert(JZ_VAL_TAG(val) == jz_tt_void);
  var = jz_unwrap_void(jz, val);
  var->index = *i;

  *i = *i + 1;
}

jz_bool assign_indices_walker(JZ_STATE, jz_cons* node, void* data) {
  assign_indices(jz, UNVOID(CADR(node), comp_state*));

  return jz_true;
}

void compile_statements(STATE, jz_cons* node) {
  while (node != NULL) {
    compile_statement(jz, state, NODE(CAR(node)));
    node = NODE(CDR(node));
  }
}

static void compile_statement(STATE, jz_cons* node) {
  jz_parse_type type;

  /* Empty statement */
  if (node == NULL)
    return;

  type = ENUM(CAR(node));
  node = NODE(CDR(node));

  switch (type) {
  case jz_parse_block:
    compile_statements(jz, state, node);
    break;

  case jz_parse_var:
    compile_vars(jz, state, node);
    break;

  case jz_parse_return:
    compile_return(jz, state, NODE(CAR(node)));
    break;
      
  case jz_parse_expr:
    compile_expr(jz, state, node, jz_false);
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
    printf("Unknown statement type %d\n", type);
    exit(1);
  }
}

void compile_vars(STATE, jz_cons* node) {
  while (node != NULL) {
    compile_var(jz, state, NODE(CAR(node)));
    node = NODE(CDR(node));
  }
}

void compile_var(STATE, jz_cons* node) {
  variable* var;
  jz_index index;
  jz_cons* expr;

  var = UNVOID(CADR(node), variable*);
  index = var->index;

  expr = NODE(CADDR(node));

  if (expr != NULL) {
    compile_expr(jz, state, expr, jz_true);
    compile_identifier_store(jz, state, var);
  }
}

void compile_return(STATE, jz_cons* node) {
  if (node == NULL)
    PUSH_OPCODE(jz_oc_end);
  else {
    compile_expr(jz, state, node, jz_true);
    PUSH_OPCODE(jz_oc_ret);
  }
}

void compile_if(STATE, jz_cons* node) {
  ptrdiff_t jump;

  compile_expr(jz, state, NODE(CAR(node)), jz_true);

  PUSH_OPCODE(jz_oc_jump_unless);
  jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

  compile_statement(jz, state, NODE(CADR(node)));

  if (NODE(CADDR(node)) == NULL) {
    jump_to_top_from(jz, state, jump);
  } else {
    ptrdiff_t else_jump;

    /* We'll want to jump past the else clause if it exists. */
    PUSH_OPCODE(jz_oc_jump);
    else_jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

    jump_to_top_from(jz, state, jump); /* Jump past the new jump instruction
                                      if the if statement fails. */


    compile_statement(jz, state, NODE(CADDR(node)));
    jump_to_top_from(jz, state, else_jump);
  }
}

void compile_do_while(STATE, jz_cons* node) {
  ptrdiff_t jump;
  jz_bool conditional_is_literal = jz_false;
  jz_val conditional_literal = get_literal_value(jz, NODE(CAR(node)),
                                                    &conditional_is_literal);

  /* If the conditional is a literal value that evaluates to true,
     we don't bother to check it. */
  jz_bool skip_conditional = conditional_is_literal &&
    jz_to_bool(jz, conditional_literal);

  jump = state->code->next - state->code->values;
  compile_statement(jz, state, NODE(CADR(node)));

  if (skip_conditional)
    PUSH_OPCODE(jz_oc_jump);
  else {
    compile_expr(jz, state, NODE(CAR(node)), jz_true);
    PUSH_OPCODE(jz_oc_jump_if);
  }

  jump_to_from_top(jz, state, jump);
}

void compile_while(STATE, jz_cons* node) {
  ptrdiff_t index, placeholder;
  jz_bool conditional_is_literal = jz_false;
  jz_val conditional_literal = get_literal_value(jz, NODE(CAR(node)),
                                                    &conditional_is_literal);

  /* If the conditional is NULL or a literal value that evaluates to true,
     we don't bother to check it.
     This makes stuff like "for (;;)" and "while (true)" faster. */
  jz_bool skip_conditional = NODE(CAR(node)) == NULL ||
    (conditional_is_literal && jz_to_bool(jz, conditional_literal));

  index = state->code->next - state->code->values;

  if (!skip_conditional) {
    compile_expr(jz, state, NODE(CAR(node)), jz_true);

    PUSH_OPCODE(jz_oc_jump_unless);
    placeholder = push_placeholder(jz, state, JZ_OCS_PTRDIFF);
  }

  compile_statement(jz, state, NODE(CADR(node)));
  PUSH_OPCODE(jz_oc_jump);
  jump_to_from_top(jz, state, index);

  if (!skip_conditional) jump_to_top_from(jz, state, placeholder);
}

void compile_for(STATE, jz_cons* node) {
  jz_cons *body, *inc_expr, *while_statement;

  if (NODE(CAR(node)) != NULL)
    compile_statement(jz, state, NODE(CAR(node)));

  inc_expr = NODE(CADDR(node));
  if (inc_expr != NULL) {
    body = jz_list(jz, 3, jz_enum_new(jz, jz_parse_block),
                   CONS(jz_enum_new(jz, jz_parse_expr), inc_expr),
                   NODE(CADDDR(node)));
  } else body = NODE(CADDDR(node));

  while_statement = jz_list(jz, 3, jz_enum_new(jz, jz_parse_while),
                                  NODE(CADR(node)), body);
  compile_statement(jz, state, while_statement);
}

void compile_switch(STATE, jz_cons* node) {
  jz_ptrdiff_vector* placeholders;

  compile_expr(jz, state, NODE(CAR(node)), jz_true);

  placeholders = compile_switch_conditionals(jz, state, NODE(CDR(node)));
  compile_switch_statements(jz, state, NODE(CDR(node)), placeholders);

  PUSH_OPCODE(jz_oc_pop);

  jz_ptrdiff_vector_free(jz, placeholders);
}

jz_ptrdiff_vector* compile_switch_conditionals(STATE, jz_cons* node) {
  jz_ptrdiff_vector* placeholders = jz_ptrdiff_vector_new(jz);

  if (node == NULL) return placeholders;

  while (node != NULL) {
    jz_cons* case_node;

    case_node = NODE(CAR(node));
    node = NODE(CDR(node));

    if (NODE(CAR(case_node)) == NULL)
      continue;

    PUSH_OPCODE(jz_oc_dup);
    compile_expr(jz, state, NODE(CAR(case_node)), jz_true);
    PUSH_OPCODE(jz_oc_strict_eq);
    PUSH_OPCODE(jz_oc_jump_if);
    jz_ptrdiff_vector_append(jz, placeholders,
                             push_placeholder(jz, state, JZ_OCS_PTRDIFF));
  }

  PUSH_OPCODE(jz_oc_jump);
  jz_ptrdiff_vector_append(jz, placeholders,
                           push_placeholder(jz, state, JZ_OCS_PTRDIFF));

  return placeholders;
}

void compile_switch_statements(STATE, jz_cons* node, jz_ptrdiff_vector* placeholders) {
  ptrdiff_t* next_placeholder = placeholders->values;
  ptrdiff_t default_pos = -1; /* -1 indicates that there is no default case. */

  if (node == NULL) return;

  while (node != NULL) {
    jz_cons* case_node = NODE(CAR(node));

    /* If this is the default case,
       we only want to jump to it after trying all other conditionals. */
    if (NODE(CAR(case_node)) == NULL)
      default_pos = state->code->next - state->code->values;
    else
      jump_to_top_from(jz, state, *(next_placeholder++));

    if (NODE(CDR(case_node)) != NULL)
      compile_statements(jz, state, NODE(CDR(case_node)));

    node = NODE(CDR(node));
  }

  if (default_pos != -1)
    jump_to_from(jz, state, default_pos, *next_placeholder);
  else
    jump_to_top_from(jz, state, *next_placeholder);
}

void compile_expr(STATE, jz_cons* node, jz_bool value) {
  jz_parse_type type = ENUM(CAR(node));
  node = NODE(CDR(node));

  switch (type) {
  case jz_parse_identifier:
    compile_identifier(jz, state, node, value);
    break;

  case jz_parse_literal:
    compile_literal(jz, state, node, value);
    break;

  case jz_parse_this:
    compile_this(jz, state, node, value);
    break;

  case jz_parse_call:
    compile_call(jz, state, node, value);
    break;

  case jz_parse_func:
    compile_func(jz, state, node, value);
    break;

  case jz_parse_cond:
    compile_cond(jz, state, node, value);
    break;

  default:
    if (JZ_PTYPE_IS_UNOP(type))
      compile_unop(jz, state, node, type, value);
    else if (JZ_PTYPE_IS_BINOP(type))
      compile_binop(jz, state, node, type, value);
    else {
      printf("Unrecognized expression node type %d\n", type);
      exit(1);
    }
  }
}

variable* compile_identifier(STATE, jz_cons* node, jz_bool value) {
  variable* var;

  /* Callers can send in any sub-expression that might be an identifier
     and compile_identifier will die if it isn't. */
  if (JZ_IS_GC_TYPE(CAR(node), jz_t_enum)) {
    if (ENUM(CAR(node)) == jz_parse_identifier)
      node = NODE(CDR(node));
    else {
      fprintf(stderr, "Invalid identifier.\n");
      exit(1);
    }
  }

  var = UNVOID(CAR(node), variable*);

  if (!value)
    return var;

  switch (var->type) {
  case local_var:
    PUSH_OPCODE(jz_oc_retrieve);
    PUSH_ARG(var->index);
    break;

  case global_var: {
    jz_index index = add_const(jz, state, var->name);

    PUSH_OPCODE(jz_oc_load_global);
    PUSH_ARG(index);
    break;
  }

  case closure_var:
    PUSH_OPCODE(jz_oc_closure_retrieve);
    PUSH_ARG(var->index);
    break;

  default:
    fprintf(stderr, "Unrecognized variable type %d\n", var->type);
    exit(1);
  }

  return var;
}

void compile_identifier_store(STATE, variable* var) {
  switch (var->type) {
  case local_var:
    PUSH_OPCODE(jz_oc_store);
    PUSH_ARG(var->index);
    break;

  case global_var: {
    jz_index index = add_const(jz, state, var->name);

    PUSH_OPCODE(jz_oc_store_global);
    PUSH_ARG(index);
    break;
  }

  case closure_var:
    PUSH_OPCODE(jz_oc_closure_store);
    PUSH_ARG(var->index);
    break;

  default:
    fprintf(stderr, "Unrecognized variable type %d\n", var->type);
    exit(1);
  }
}

void compile_literal(STATE, jz_cons* node, jz_bool value) {
  jz_index index = add_const(jz, state, CAR(node));

  if (!value)
    return;

  PUSH_OPCODE(jz_oc_push_literal);
  PUSH_ARG(index);
}

void compile_this(STATE, jz_cons* node, jz_bool value) {
  if (!value)
    return;

  PUSH_OPCODE(jz_oc_push_global);
}

#define SIMPLE_UNOP_CASE(operator, opcode)              \
  case operator: {                                      \
    compile_expr(jz, state, node, jz_true);             \
    PUSH_OPCODE(opcode);                                \
                                                        \
    if (!value)                                         \
      PUSH_OPCODE(jz_oc_pop);                           \
                                                        \
    break;                                              \
  }

void compile_unop(STATE, jz_cons* node, jz_parse_type op, jz_bool value) {
  node = NODE(CAR(node));

  switch (op) {
  SIMPLE_UNOP_CASE(jz_op_un_add, jz_oc_to_num)
  SIMPLE_UNOP_CASE(jz_op_un_sub, jz_oc_neg)
  SIMPLE_UNOP_CASE(jz_op_bw_not, jz_oc_bw_not)
  SIMPLE_UNOP_CASE(jz_op_not,    jz_oc_not)

  case jz_op_pre_inc:
    compile_unit_shortcut(jz, state, node, jz_oc_add, jz_true, value);
    break;

  case jz_op_pre_dec:
    compile_unit_shortcut(jz, state, node, jz_oc_sub, jz_true, value);
    break;

  case jz_op_post_inc:
    compile_unit_shortcut(jz, state, node, jz_oc_add, jz_false, value);
    break;

  case jz_op_post_dec:
    compile_unit_shortcut(jz, state, node, jz_oc_sub, jz_false, value);
    break;

  default:
    printf("Unrecognized unary operator %d\n", op);
    exit(1);
  }
}

/* TODO: a.b++? */
static void compile_unit_shortcut(STATE, jz_cons* node,
                                  jz_opcode op, jz_bool pre, jz_bool value) {
  jz_index unit_index = add_const(jz, state, jz_wrap_num(jz, 1));
  variable* var = compile_identifier(jz, state, node, jz_true);

  if (!pre && value)
    PUSH_OPCODE(jz_oc_dup);

  PUSH_OPCODE(jz_oc_push_literal);
  PUSH_ARG(unit_index);
  PUSH_OPCODE(op);

  if (pre && value)
    PUSH_OPCODE(jz_oc_dup);

  compile_identifier_store(jz, state, var);
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

void compile_binop(STATE, jz_cons* node, jz_parse_type op, jz_bool value) {
  switch (op) {
  case jz_op_comma:
    compile_comma(jz, state, node, value);
    break;

  case jz_op_and:
  case jz_op_or:
    compile_logical_binop(jz, state, node, op, value);
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
    fprintf(stderr, "Unknown operator %d\n", op);
    exit(1);
  }
}

void compile_comma(STATE, jz_cons* node, jz_bool value) {
  compile_expr(jz, state, NODE(CAR(node)), jz_false);
  compile_expr(jz, state, NODE(CADR(node)), value);
}

void compile_logical_binop(STATE, jz_cons* node, jz_parse_type op, jz_bool value) {
  ptrdiff_t jump;

  compile_expr(jz, state, NODE(CAR(node)), jz_true);

  if (value)
    PUSH_OPCODE(jz_oc_dup);

  PUSH_OPCODE(op == jz_op_or ? jz_oc_jump_if : jz_oc_jump_unless);
  jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

  if (value)
    PUSH_OPCODE(jz_oc_pop);

  compile_expr(jz, state, NODE(CADR(node)), value);
  jump_to_top_from(jz, state, jump);
}

void compile_simple_binop(STATE, jz_cons* node, jz_opcode op, jz_bool value) {
  compile_expr(jz, state, NODE(CAR(node)), jz_true);
  compile_expr(jz, state, NODE(CADR(node)), jz_true);

  if (op != jz_oc_noop)
    PUSH_OPCODE(op);
  else if (!value)
    PUSH_OPCODE(jz_oc_pop);
}

/* TODO: This can be compiled to ands and ors. */
void compile_cond(STATE, jz_cons* node, jz_bool value) {
  ptrdiff_t cond_jump, branch1_jump;

  compile_expr(jz, state, NODE(CAR(node)), jz_true);
  PUSH_OPCODE(jz_oc_jump_unless);
  cond_jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);

  compile_expr(jz, state, NODE(CADR(node)), value);

  PUSH_OPCODE(jz_oc_jump);
  branch1_jump = push_placeholder(jz, state, JZ_OCS_PTRDIFF);
  jump_to_top_from(jz, state, cond_jump);

  compile_expr(jz, state, NODE(CADDR(node)), value);
  jump_to_top_from(jz, state, branch1_jump);

}

void compile_call(STATE, jz_cons* node, jz_bool value) {
  jz_index arg_count = 0;
  jz_cons* arg = NODE(CDR(node));

  compile_expr(jz, state, NODE(CAR(node)), jz_true);

  while (arg != NULL) {
    compile_expr(jz, state, NODE(CAR(arg)), jz_true);
    arg_count++;
    arg = NODE(CDR(arg));
  }

  PUSH_OPCODE(jz_oc_call);
  PUSH_ARG(arg_count);

  if (!value)
    PUSH_OPCODE(jz_oc_pop);
}

void compile_func(STATE, jz_cons* node, jz_bool value) {
  jz_bytecode* code;
  jz_index index;

  if (!value)
    /* TODO: Once function exprs can be named,
       this won't always be appropriate. */
    return;

  code = compile(jz, UNVOID(CAR(node), comp_state*), NODE(CADR(node)));

  index = add_const(jz, state, jz_func_new(jz, code, 0));
  PUSH_OPCODE(jz_oc_push_closure);
  PUSH_ARG(index);
}

void compile_assign_binop(STATE, jz_cons* node, jz_opcode op, jz_bool value) {
  jz_parse_type left = ENUM(CAAR(node));

  if (left == jz_parse_identifier)
    compile_identifier_assign(jz, state, node, op, value);
  else if (left == jz_op_index)
    compile_index_assign(jz, state, node, op, value);
  else {
    fprintf(stderr, "Invalid left-hand side of assignment.\n");
    exit(1);
  }
}

void compile_identifier_assign(STATE, jz_cons* node, jz_opcode op, jz_bool value) {
  jz_cons* left = NODE(CAR(node));
  jz_cons* right = NODE(CADR(node));
  variable* var;

  ASSERT_GC_TYPE(CAR(left), jz_t_enum);
  assert(ENUM(CAR(left)) == jz_parse_identifier);

  /* Noop signals that this is just a plain assignment.
     Otherwise we want to run an operation before assigning. */
  if (op == jz_oc_noop) {
    assert(JZ_VAL_TAG(CADR(left)) == jz_tt_void);

    var = UNVOID(CADR(left), variable*);
    compile_expr(jz, state, right, jz_true);
  } else {
    var = compile_identifier(jz, state, left, jz_true);

    compile_expr(jz, state, right, jz_true);
    PUSH_OPCODE(op);
  }
  
  if (value)
    PUSH_OPCODE(jz_oc_dup);

  compile_identifier_store(jz, state, var);
}

void compile_index_assign(STATE, jz_cons* node, jz_opcode op, jz_bool value) {
  jz_cons* left = NODE(CDAR(node));
  jz_cons* right = NODE(CADR(node));
  char base_stack_size = 2;

  compile_simple_binop(jz, state, left, jz_oc_noop, jz_true);

  /* Noop signals that this is just a plain assignment.
     Otherwise we want to run an operation before assigning. */
  if (op != jz_oc_noop) {
    PUSH_OPCODE(jz_oc_dup2);
    PUSH_OPCODE(jz_oc_index);
    base_stack_size++;
  }

  compile_expr(jz, state, right, jz_true);

  if (op != jz_oc_noop)
    PUSH_OPCODE(op);

  if (value) {
    PUSH_OPCODE(jz_oc_dup);
    PUSH_OPCODE(jz_oc_rot4);
  }

  PUSH_OPCODE(jz_oc_index_store);
}

/* Get the jz_val of a jz_parse_exprs node if it's just a literal value,
   or NULL if it's not. */
jz_val get_literal_value(JZ_STATE, jz_cons* node, jz_bool* success) {
  success = jz_false;

  if (node == NULL ||
      !JZ_IS_GC_TYPE(CAR(node), jz_t_enum) ||
      ENUM(CAR(node)) != jz_parse_literal)
    return NULL;

  return CADR(node);
}

variable* get_var(STATE, jz_str* name, jz_bool from_inner_scope) {
  variable* var;

  /* Are we at the end of the scope chain? */
  if (state == NULL) {
    /* TODO: Store these in some sort of data structure
       so that they can be freed later on. */
    var = malloc(sizeof(variable));
    var->type = global_var;
    var->name = name;
    var->index = 0;

    /* Precompute hash code */
    jz_str_hash(jz, var->name);
    return var;    
  }

  var = jz_obj_get_ptr(jz, state->local_vars, name);
  if (var != NULL) {
    if (from_inner_scope) {
      /* We want to make this a closure var */
      jz_obj_remove(jz, state->local_vars, name);
      jz_obj_put_ptr(jz, state->closure_vars, name, var);
      var->type = closure_var;
    }

    return var;
  }

  /* Not a local, try closure vars. */
  var = jz_obj_get_ptr(jz, state->closure_vars, name);
  if (var != NULL)
    return var;

  /* Neither local nor closure, either defined above or global. */
  return get_var(jz, state->scope, name, jz_true);
}

variable* add_lvar(STATE, jz_str* name) {
  variable* node;

  if ((node = jz_obj_get_ptr(jz, state->local_vars, name)))
    return node;

  node = malloc(sizeof(variable));
  node->type = local_var;
  node->name = name;
  node->index = state->local_vars->size;
  jz_obj_put_ptr(jz, state->local_vars, name, node);

  return node;
}

jz_index add_const(STATE, jz_val value) {
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
  jz_obj_each(jz, state->local_vars, free_variable, NULL);
  jz_obj_each(jz, state->closure_vars, free_variable, NULL);

  while (state->consts != NULL) {
    const_node* old_consts = state->consts;
    state->consts = state->consts->next;
    free(old_consts);
  }

  jz_opcode_vector_free(jz, state->code);
  free(state);
}

void free_variable(JZ_STATE, jz_str* key, jz_val val, void* data) {
  free(jz_unwrap_void(jz, val));
}

void jz_free_bytecode(JZ_STATE, jz_bytecode* this) {
  if (this == NULL) return;

  free(this->code);
  free(this->consts);
  free(this);
}
