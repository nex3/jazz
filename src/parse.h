#ifndef JZ_PARSE_H
#define JZ_PARSE_H

#include "string.h"

typedef enum {
  jz_parse_identifier
} jz_parse_type;

typedef struct jz_parse_node jz_parse_node;

typedef union {
  jz_parse_node* node;
} jz_parse_value;

struct jz_parse_node {
  jz_parse_type  type;
  jz_parse_value car;
  jz_parse_value cdr;
};

jz_parse_node* jz_parse_string(jz_str code);

#endif
