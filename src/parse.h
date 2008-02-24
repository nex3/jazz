#ifndef JZ_PARSE_H
#define JZ_PARSE_H

#include "jazz.h"
#include "value.h"
#include "gc.h"
#include "cons.h"

/* When updating this, don't forget to update jz_parse_names in parse.y */
typedef enum {
  jz_parse_block,      /* A block statement consisting of multiple sub-statements.
                          cdr.node is the list of sub-statements. */
  jz_parse_return,     /* A return statement.
                          cadr.node is the expression to be returned,
                          or NULL if there is no expression being returned. */
  jz_parse_if,         /* An if statement.
                          cadr.node is the conditional expression.
                          caddr.node is the statement to evaluate
                          if the conditional evaluates to true.
                          cadddr.node is the statement to evaluate otherwise,
                          or NULL if none was given. */
  jz_parse_do_while,   /* A do-while statement.
                          cadr.node is the conditional expression.
                          caddr.node is the statement to evaluate
                          while the conditional evaluates to true. */
  jz_parse_while,      /* A while statement.
                          cadr.node is the conditional expression.
                          caddr.node is the statement to evaluate
                          while the conditional evaluates to true. */
  jz_parse_for,        /* A for statement.
                          cadr.node is the initializer statement, or NULL.
                          caddr.node is the conditional expression, or NULL.
                          cadddr.node is the increment expression, or NULL.
                          caddddr.node is the statement list to evaluate. */
  jz_parse_switch,     /* A switch statement.
                          cadr.node is the expression being switched on.
                          cddr.node is a list of cases, where for each case:
                          car.node is that case's expression
                          (or NULL for the default case).
                          cdr.node is that case's list of statements. */
  jz_parse_var,        /* A variable initialization statement.
                          cdr.node is a list of individual declarations,
                          where for each declaration:
                          car.str is the name of the variable.
                          cadr.node is the expression initializer of the variable,
                          or NULL if there is no initializer. */

  jz_parse_expr,       /* An expression statement.
                          cdr.node is the expression. */
  jz_parse_literal,    /* A literal value.
                          cadr.val is the value. */
  jz_parse_identifier, /* An identifier.
                          cadr.str is the name. */
  jz_parse_this,       /* The `this' keyword.
                          
                          TODO: Would it be too inconsistent to not wrap this in a cons?
                          What would the compiler code look like? */
  jz_parse_call,       /* A function call.
                          cadr.node is the function,
                          cddr.node is a list of arguments. */
  jz_parse_func,       /* A function declaration.
                          cadr.node is a jz_parse_statements
                          containing the source code of the function. */
  jz_parse_cond,       /* The ?: conditional operator.
                          cadr.node is the conditional expression,
                          caddr.node is the true option,
                          and cadddr.node is the false option. */

  /*** Operator parse nodes. These are all expressions. ***/

  /* Binary operators.
     cadr.node and caddr.node are the operands. */
  jz_op_or,
  jz_op_and,
  jz_op_comma,
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
  jz_op_index,

  /* Unary operators.
     cadr.node is the only operand. */
  jz_op_un_add,
  jz_op_un_sub,
  jz_op_bw_not,
  jz_op_not,
  jz_op_pre_inc,
  jz_op_pre_dec,
  jz_op_post_inc,
  jz_op_post_dec,

  jz_parse_last
} jz_parse_type;

const char* jz_parse_names[jz_parse_last];

#define JZ_PTYPE_IS_UNOP(type) ((type) >= jz_op_un_add)
#define JZ_PTYPE_IS_BINOP(type) \
  ((type) >= jz_op_or && (type) <= jz_op_un_add)

/* Parses a Javascript program and returns its parse tree.
   The root node of the parse tree is jz_parse_statements. */
jz_cons* jz_parse_string(JZ_STATE, const jz_str* code);

#endif
