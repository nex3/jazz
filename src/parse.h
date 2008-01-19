/* The Jazz parser.
   The parser uses yacc combined with a custom lexer (see lex.*).
   It generates an abstract syntax tree
   in the form of jz_parse_node s-expressions.
   These are mainly meant to be read (in particular by compile.c),
   but can be created or otherwise manipulated
   via the jz_pnode and jz_plist functions.

   When creating sexps, though, proceed with caution.
   We have to do some nasty tricks to get dynamic typing working smoothly,
   and this means that compile-time typechecks
   on args to jz_pnode_(list|cons|wrap) and similar functions don't exist.
   Make sure these arguments are always members of jz_parse_value,
   and always pointers rather than direct values.

   In addition, the C types of each sexp's car and cdr
   are determined by that sexp's jz_parse_type.
   This helps conserve memory by not wasting it on all sorts of type flags,
   but it also means that we have to be careful.
   Pay close attention to the documentation for each jz_parse_type
   to see what the types of its car and cdr are.

   Note that we use vaguely Lisp-y notation,
   since we're dealing with s-expressions.
   Here's a brief glossary for those unfamiliar with the terminology:

     s-expression: A tree-like data structure made up of cons cells.

     cons cell: A tree node with two pointers,
       known as its "car" and "cdr."
       This corresponds to the jz_parse_node struct.
       Note that our cons cells have explicit types,
       and the car and cdr have types implicitly defined by the cons cell.

     list: An ordered set represented as an s-expression.
       A list is made by treating cons cells as a linked list.
       It's a series of cons cells, all of the same type,
       where each cell's cdr points to the next cell in the list.
       The last cell's cdr points to NULL, indicating the end of the list.
       Then the cars are the elements of the list.

     cadr, cddar, etc: Shorthands for multiple cars and cdrs.
       For each "a" between the "c" and the "r", one "car" is applied,
       and for each "d", one "cdr" is applied.
       For instance, cddar(node) is the same as node->cdr.node->cdr.node->car. */

#ifndef JZ_PARSE_H
#define JZ_PARSE_H

#include "string.h"
#include "type.h"

typedef struct jz_parse_node jz_parse_node;

/* These are the possible node types for parse tree nodes.

   In these descriptions, saying that a node is a "list" means that
   it is a node in a list as defined above.
   Only the semantics of the car of the node needs to be defined;
   cdr is always the next node in the list or NULL.

   Note that some of the descriptions refer to "expressions"
   and some to "statements."
   An "expression" is always a node of type jz_parse_exprs,
   but a "statement" can be a node of any of the following types:
     jz_parse_empty, jz_parse_statements, jz_parse_vars, jz_parse_return,
     jz_parse_exprs, jz_parse_if, jz_parse_do_while, jz_parse_while,
     jz_parse_for, jz_parse_switch.

   When compiling to bytecode,
   an expression can be compiled using the compile_exprs function in compile.c,
   and a statement can be compiled using compile_statement.
   See compile.c for more information. */
typedef enum {
  jz_parse_cont,       /* A continuation node.
                          Semantics are defined by the first
                          non-continuation parent node. */

  jz_parse_statements, /* A list of statements.
                          car.node is a statement. */

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

/* Operator types for unary, binary, and trinary operators. */
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

/* Potential values for the car and cdr of a jz_parse_node.

   Note that all of these should be pointers;
   this way, they can be guaranteed to all be the same size,
   which is necessary for some dynamic typing facilities to work.
   This means, unfortunately, that members that should really be value types,
   such as 'val' and 'op_type' can't be. */
typedef union {
  jz_parse_node* node;
  jz_str* str;
  jz_tvalue* val;
  jz_op_type* op_type;
} jz_parse_value;

/* A node in the parse tree.
   The types of 'car' and 'cdr' are defined by 'type'.
   See the documentation for jz_parse_type. */
struct jz_parse_node {
  jz_parse_type  type;
  jz_parse_value car;
  jz_parse_value cdr;
};

/* Parses a Javascript program and returns its parse tree.
   The root node of the parse tree is jz_parse_statements. */
jz_parse_node* jz_parse_string(const jz_str* code);


/* Returns a new parse node of the given type,
   with 'argc' values given as variadic arguments.
   The first such value is the car of the node,
   the second is the cdar, the third is the cddar, and so forth,
   except that the last argument is the cdr of the last node.
   Thus, jz_node_list(type, 3, foo, bar, baz) has
     car = foo, cdar = bar, cddr = baz
   and jz_node_list(type, 2, foo, bar) has
     car = foo, cdr = bar. */
jz_parse_node* jz_pnode_list(jz_parse_type type, int argc, ...);

/* Returns a new parse node of the given type with NULL car and cdr. */
jz_parse_node* jz_pnode_new(jz_parse_type type);

/* Concatenates two lists.
   list1 is modified so that its last element
   points to the first element of list2.
   No new nodes are allocated.

   list1 is returned as a convenience, unless it's NULL,
   in which case list2 is returned. */
jz_parse_node* jz_plist_concat(jz_parse_node* list1, jz_parse_node* list2);

#define jz_pnode_cons(type, car, cdr) jz_pnode_list(type, 2, car, cdr)
#define jz_pnode_wrap(type, car)      jz_pnode_list(type, 1, car)

#endif
