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

#define JZ_NUM_IS_NAN(num) ((num) != (num))
#define JZ_NUM_IS_INF(num) ((num) == (num) + 1)

jz_tvalue jz_wrap_num(double num);
jz_tvalue jz_wrap_bool(bool b);

double jz_to_num(jz_tvalue val);
bool jz_to_bool(jz_tvalue val);
int jz_to_int32(jz_tvalue val);

#endif
