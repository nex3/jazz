#ifndef JZ_PARSE_H
#define JZ_PARSE_H

#include "string.h"

typedef struct jz_parse_node jz_parse_node;

typedef enum {
  jz_parse_cont,  /* A continuation node.
                     Semantics are defined by the first
                     non-continuation parent node. */
  jz_parse_binop, /* A binary operator.
                     car.op_type indicates which operator it is,
                     cdar.node is the left-hand argument,
                     cddr.node is the right-hand argument. */
  jz_parse_triop, /* A trinary operator.
                     car.op_type indicates which operator it is,
                     cdar.node is the first argument,
                     cddar.node is the second argument, 
                     cdddr.node is the third argument. */
  jz_parse_num    /* A number literal.
                     car.num is the value. */
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
  jz_op_mod
} jz_op_type;

typedef union {
  jz_parse_node* node;
  double num;
  jz_op_type op_type;
} jz_parse_value;

struct jz_parse_node {
  jz_parse_type  type;
  jz_parse_value car;
  jz_parse_value cdr;
};

jz_parse_node* jz_parse_string(jz_str code);

#endif
