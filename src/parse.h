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
  jz_parse_num    /* A number literal.
                     car.num is the value. */
} jz_parse_type;

typedef enum {
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
