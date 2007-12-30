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
                          Each node's car.node is a jz_parse_statement,
                          and cdr.node is the next link in the list.
                          The last link's cdr.node is NULL.
                          Note that because the grammar is left-recursive,
                          the top node is actually the last statement. */

  jz_parse_statement,  /* A statement.
                          car.st_type is the type of statement.
                          cdr is defined per-statement-type. */
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
  jz_op_eq_eq_eq,
  jz_op_not_eq_eq,
  jz_op_lt,
  jz_op_gt,
  jz_op_lt_eq,
  jz_op_gt_eq,
  jz_op_lt_lt,
  jz_op_gt_gt,
  jz_op_gt_gt_gt,
  jz_op_plus,
  jz_op_minus,
  jz_op_times,
  jz_op_div,
  jz_op_mod,
  jz_op_assign,
  jz_op_bw_not,
  jz_op_not
} jz_op_type;

typedef enum {
  jz_st_empty, /* cdr isn't used. */
  jz_st_var,   /* cdr.node is a jz_parse_vars. */
  jz_st_expr,  /* cdr.node is a jz_parse_exprs. */
  jz_st_return /* cdr.node is the jz_parse_exprs to be returned,
                  or NULL if there is no expression being returned. */
} jz_st_type;

typedef union {
  jz_parse_node* node;
  jz_tvalue val;
  jz_str* str;
  jz_op_type op_type;
  jz_st_type st_type;
} jz_parse_value;

struct jz_parse_node {
  jz_parse_type  type;
  jz_parse_value car;
  jz_parse_value cdr;
};

jz_parse_node* jz_parse_string(jz_str* code);

#endif
