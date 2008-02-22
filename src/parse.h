#ifndef JZ_PARSE_H
#define JZ_PARSE_H

#include "jazz.h"
#include "value.h"
#include "gc.h"

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

  jz_parse_expr,       /* An expression.
                          cdr.node is a list of sub-expressions.

                          TODO: See if we can't get rid of this. */
  jz_parse_unop,       /* A unary operator.
                          cadr.leaf indicates which operator it is,
                          caddr.node is the argument. */
  jz_parse_binop,      /* A binary operator.
                          cadr.leaf indicates which operator it is,
                          caddr.node is the left-hand argument,
                          cadddr.node is the right-hand argument. */
  jz_parse_triop,      /* A trinary operator.
                          cadr.leaf indicates which operator it is,
                          caddr.node is the first argument,
                          cadddr.node is the second argument, 
                          caddddr.node is the third argument. */
  jz_parse_literal,    /* A literal value.
                          cadr.val is the value. */
  jz_parse_identifier, /* An identifier.
                          cadr.str is the name. */
  jz_parse_this,       /* The `this' keyword.
                          
                          TODO: Would it be too inconsistent to not wrap this in a pnode?
                          What would the compiler code look like? */
  jz_parse_call,       /* A function call.
                          cadr.node is the function,
                          cddr.node is a list of arguments. */
  jz_parse_func        /* A function declaration.
                          cadr.node is a jz_parse_statements
                          containing the source code of the function. */
} jz_parse_type;

/* Operator types for unary, binary, and trinary operators.
   TODO: Separate based on arg number, get rid of jz_parse_{un,bin,tri}op. */
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
  jz_op_post_dec,
  jz_op_index
} jz_op_type;

typedef struct jz_parse_node jz_parse_node;

/* Parse tree leaf node.
   Can hold enum values.
   tvalues can be used directly,
   as they're already tagged. */
typedef struct jz_parse_leaf {
  jz_tag tag;
  unsigned char val;
} jz_parse_leaf;

/* Potential values for the car and cdr of a jz_parse_node.

   Note that all of these should be pointers to values
   where the first element is a jz_tag.

   Also, the Jazz null value isn't a pointer to a null tvalue,
   but rather an actual NULL pointer.
   This avoids ambiguity between null tvalues and objects. */
typedef union {
  jz_tag* tag;
  jz_gc_header* gc;
  jz_parse_node* node;
  jz_str* str;
  jz_tvalue* val;
  jz_parse_leaf* leaf;
} jz_parse_ptr;

/* A node in the parse tree.
   The types of 'car' and 'cdr' are defined by 'type'.
   See the documentation for jz_parse_type. */
struct jz_parse_node {
  jz_gc_header gc;
  jz_parse_ptr car;
  jz_parse_ptr cdr;
};

/* Parses a Javascript program and returns its parse tree.
   The root node of the parse tree is jz_parse_statements. */
jz_parse_node* jz_parse_string(JZ_STATE, const jz_str* code);

jz_parse_node* jz_pnode_list(JZ_STATE, int argc, ...);

/* Returns a new parse node with NULL car and cdr. */
jz_parse_node* jz_pnode_new(JZ_STATE);

jz_parse_leaf* jz_pleaf_new(JZ_STATE, unsigned char val);

jz_parse_node* jz_pnode_cons(JZ_STATE, jz_tag* car, jz_tag* cdr);

/* Wraps a jz_tvalue in a jz_parse_ptr.
   This pointer may need to be freed later on -
   jz_free_parse_tree in compile.h handles this.

   Note that this doesn't necessarily return a pointer to a jz_tvalue;
   if the tvalue is itself a pointer to a garbage-collectable object,
   that pointer will be returned instead. */
jz_parse_ptr jz_tval_to_pleaf(JZ_STATE, jz_tvalue val);

jz_tvalue jz_pleaf_to_tval(JZ_STATE, jz_parse_ptr ptr);

/* Concatenates two lists.
   list1 is modified so that its last element
   points to the first element of list2.
   No new nodes are allocated.

   list1 is returned as a convenience, unless it's NULL,
   in which case list2 is returned. */
jz_parse_node* jz_plist_concat(JZ_STATE, jz_parse_node* list1, jz_parse_node* list2);

void jz_print_parse_tree(JZ_STATE, jz_parse_node* root);

void jz_pnode_free(JZ_STATE, jz_parse_node* this);

#define jz_pnode_pair(jz, type, car, cdr) jz_pnode_list(jz, 3, jz_pleaf_new(jz, type), car, cdr)
#define jz_pnode_wrap(jz, type, car)      jz_pnode_list(jz, 2, jz_pleaf_new(jz, type), car)

#endif
