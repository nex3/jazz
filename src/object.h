#ifndef JZ_OBJECT_H
#define JZ_OBJECT_H

#include <stdbool.h>

enum {
  jz_num,
  jz_bool
} jz_type_type;

typedef union {
  double num;
  bool b;
} jz_value;

typedef struct {
  unsigned char type;
  jz_value value;
} jz_tvalue;

jz_tvalue jz_wrap_num(double num);\
jz_tvalue jz_wrap_bool(bool b);

#endif
