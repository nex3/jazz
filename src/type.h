#ifndef JZ_OBJECT_H
#define JZ_OBJECT_H

#include <stdbool.h>

enum {
  /* Giving undefined a 0 flag means that zero-ed memory
     is identified as jz_undef, which saves some manual setting. */
  jz_undef = 0x00,
  jz_num   = 0x01,
  jz_bool  = 0x02
} jz_type_type;

typedef union {
  double num;
  bool b;
} jz_value;

typedef struct {
  unsigned char tag;
  jz_value value;
} jz_tvalue;

#define JZ_NEG_0   (-0.0)
#define JZ_INF     (1.0/0.0)
#define JZ_NEG_INF (-1.0/0.0)
#define JZ_NAN     (0.0/0.0)

#define JZ_TVAL_TYPE(value)           ((value).tag & 0x03)
#define JZ_TVAL_SET_TYPE(value, type) ((value).tag = ((value).tag & !0x03) | type)

#define JZ_NUM_IS_NAN(num)     ((num) != (num))
#define JZ_NUM_IS_INF(num)     ((num) == (num) + 1)
#define JZ_NUM_IS_POS_0(num)   (1.0/(num) == JZ_INF)
#define JZ_NUM_IS_NEG_0(num)   (1.0/(num) == JZ_NEG_INF)

bool jz_values_equal(jz_tvalue v1, jz_tvalue v2);
bool jz_values_strict_equal(jz_tvalue v1, jz_tvalue v2);
double jz_values_comp(jz_tvalue v1, jz_tvalue v2);

jz_tvalue jz_wrap_num(double num);
jz_tvalue jz_wrap_bool(bool b);
jz_tvalue jz_undef_val();

double jz_to_num(jz_tvalue val);
bool jz_to_bool(jz_tvalue val);
int jz_to_int32(jz_tvalue val);
unsigned int jz_to_uint32(jz_tvalue val);

double jz_num_mod(jz_tvalue val1, jz_tvalue val2);

#endif
