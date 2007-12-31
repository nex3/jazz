#ifndef JZ_PARSE_H
#define JZ_PARSE_H

#include "string.h"
#include "type.h"

typedef struct jz_parse_node jz_parse_node;

typedef enum {
  jz_parse_cont,       /* A continuation node.
                          Semantics are defined by the first
                          non-continuation parent node. */

  jz_parse_statements, /* The beginning node of a list of statements.
                          Each node's car.node is a statement,
                          and cdr.node is the next link in the list.
                          The last link's cdr.node is NULL.
                          Note that because the grammar is left-recursive,
                          the top node is actually the last statement. */

  jz_parse_return,     /* A return statement.
                          car.node is the jz_parse_exprs to be returned,
                          or NULL if there is no expression being returned. */
  jz_parse_empty,      /* An empty statement.
                          car and cdr aren't used. */
  jz_parse_if,         /* An if statement.
                          car.node is the conditional expression.
                          cdar.node is the statement to evaluate
                          if the conditional evaluates to true.
                          cddr.node is the statement to evaluate otherwise,
                          or NULL if none was given. */
  jz_parse_do_while,   /* A do-while statement.
                          car.node is the conditional expression.
                          cdr.node is the statement to evaluate
                          while the conditional evaluates to true. */
  jz_parse_while,      /* A while statement.
                          car.node is the conditional expression.
                          cdr.node is the statement to evaluate
                          while the conditional evaluates to true. */
  jz_parse_for,        /* A for statement.
                          car.node is the initializer statement, or NULL.
                          cdar.node is the conditional expression, or NULL.
                          cddar.node is the increment expression, or NULL.
                          cdddr.node is the list of statements to evaluate. */
  jz_parse_switch,     /* A switch statement.
                          car.node is the expression being switched on.
                          cdr.node is a jz_parse_cases,
                          or NULL if there are no cases. */
  jz_parse_cases,      /* A list of switch statement cases.
                          car.node is a jz_parse_case. */
  jz_parse_case,       /* A single switch statement case.
                          car.node is the expression,
                          or NULL if this is the default case.
                          cdr.node is the statement list (a jz_parse_statements)
                          or NULL if there is no statement list. */

  jz_parse_vars,       /* A list of variable declarations.
                          car.node is a jz_parse_var. */
  jz_parse_var,        /* A single variable initialization.
                          car.str is the name of the variable.
                          cdr.node is the initializer of the variable,
                          or NULL if there is no initializer. */

  jz_parse_exprs,      /* A list of expressions.
                          car.node is the root node of an expression. */
  jz_parse_unop,       /* A unary operator.
                          car.op_type indicates which operator it is,
                          cdr.node is the argument. */
  jz_parse_binop,      /* A binary operator.
                          car.op_type indicates which operator it is,
                          cdar.node is the left-hand argument,
                          cddr.node is the right-hand argument. */
  jz_parse_triop,      /* A trinary operator.
                          car.op_type indicates which operator it is,
                          cdar.node is the first argument,
                          cddar.node is the second argument, 
                          cdddr.node is the third argument. */
  jz_parse_literal,    /* A literal value.
                          car.val is the value. */
  jz_parse_identifier  /* An identifier.
                          car.str is the name. */
} jz_parse_type;

typedef enum {
  jz_op_cond,
  jz_op_or,
  jz_op_and,
  jz_op_bw_or,
  jz_op_xor,
  jz_op_bw_and,
  jz_op_equals,
  jz_op_not_eq,
  jz_op_strict_eq,
  jz_op_not_strict_eq,
  jz_op_lt,
  jz_op_gt,
  jz_op_lt_eq,
  jz_op_gt_eq,
  jz_op_lshift,
  jz_op_rshift,
  jz_op_urshift,
  jz_op_add,
  jz_op_sub,
  jz_op_times,
  jz_op_div,
  jz_op_mod,
  jz_op_assign,
  jz_op_times_eq,
  jz_op_div_eq,
  jz_op_mod_eq,
  jz_op_add_eq,
  jz_op_sub_eq,
  jz_op_lshift_eq,
  jz_op_rshift_eq,
  jz_op_urshift_eq,
  jz_op_bw_and_eq,
  jz_op_xor_eq,
  jz_op_bw_or_eq,
  jz_op_bw_not,
  jz_op_not,
  jz_op_pre_inc,
  jz_op_pre_dec,
  jz_op_post_inc,
  jz_op_post_dec
} jz_op_type;

typedef union {
  jz_parse_node* node;
  jz_str* str;
  jz_tvalue* val;
  jz_op_type* op_type;
} jz_parse_value;

struct jz_parse_node {
  jz_parse_type  type;
  jz_parse_value car;
  jz_parse_value cdr;
};

jz_parse_node* jz_parse_string(jz_str* code);

jz_parse_node* jz_pnode_list(jz_parse_type type, int argc, ...);
jz_parse_node* jz_pnode_new(jz_parse_type type);

jz_parse_node* jz_plist_concat(jz_parse_node* list1, jz_parse_node* list2);

#define jz_pnode_cons(type, car, cdr) jz_pnode_list(type, 2, car, cdr)
#define jz_pnode_wrap(type, car)      jz_pnode_list(type, 1, car)

#endif
