#ifndef JZ_VALUE_H
#define JZ_VALUE_H

#include <stdbool.h>
#include <math.h>
#include <float.h>

#include "jazz.h"

typedef enum {
  /* Giving undefined a 0 flag means that zero-ed memory
     is identified as jz_t_undef, which saves some manual setting. */
  jz_t_undef = 0x00,
  jz_t_num,
  jz_t_bool,
  /* All type flags past this point
     must refer to types with jz_gc_header headers. */
  jz_t_str,
  jz_t_str_value,
  jz_t_obj
} jz_type;

typedef union {
  double num;
  bool b;
  jz_gc_header* gc;
  jz_obj* obj;
  jz_str* str;
} jz_value;

typedef struct {
  unsigned char tag;
  jz_value value;
} jz_tvalue;

#define JZ_TVAL_TYPE(value) ((value).tag & 0x0f)
#define JZ_TVAL_SET_TYPE(value, type) \
  ((value).tag = ((value).tag & !0x0f) | type)

#define JZ_TVAL_CAN_BE_GCED(val) \
  (JZ_TVAL_TYPE(val) >= jz_t_str && (val).value.gc != NULL)

#define JZ_TVAL_IS_NULL(val) \
  (JZ_TVAL_TYPE(val) == jz_t_obj && (val).value.obj == NULL)

#define JZ_NEG_0   (-0.0)
#define JZ_INF     (1.0/0.0)
#define JZ_NEG_INF (-1.0/0.0)
#define JZ_NAN     (0.0/0.0)

#define JZ_NUM_IS_NAN(num)         ((num) != (num))
#define JZ_NUM_IS_INF(num)         ((num) == (num) + 1)
#define JZ_NUM_IS_POS_0(num)       (1.0/(num) == JZ_INF)
#define JZ_NUM_IS_NEG_0(num)       (1.0/(num) == JZ_NEG_INF)
#define JZ_NUM_WITHIN_EPS(num)     ((num) < DBL_EPSILON && (num) > -DBL_EPSILON)
#define JZ_NUMS_WITHIN_EPS(n1, n2) (JZ_NUM_WITHIN_EPS(n1 - n2))
#define JZ_NUM_IS_INT(num)         (JZ_NUMS_WITHIN_EPS(num, floor(num)))

bool jz_values_equal(JZ_STATE, jz_tvalue v1, jz_tvalue v2);
bool jz_values_strict_equal(JZ_STATE, jz_tvalue v1, jz_tvalue v2);
double jz_values_comp(JZ_STATE, jz_tvalue v1, jz_tvalue v2);

#define jz_to_wrapped_num(jz, val) \
  (jz_wrap_num(jz, jz_to_num(jz, val)))
#define jz_to_wrapped_bool(jz, val) \
  (jz_wrap_bool(jz, jz_to_bool(jz, val)))
#define jz_to_wrapped_str(jz, val) \
  (jz_wrap_str(jz, jz_to_str(jz, val)))

jz_tvalue jz_wrap_num(JZ_STATE, double num);
jz_tvalue jz_wrap_bool(JZ_STATE, bool b);
jz_tvalue jz_wrap_obj(JZ_STATE, jz_obj* obj);
jz_tvalue jz_wrap_str(JZ_STATE, jz_str* str);

double jz_to_num(JZ_STATE, jz_tvalue val);
jz_str* jz_to_str(JZ_STATE, jz_tvalue val);
bool jz_to_bool(JZ_STATE, jz_tvalue val);
int jz_to_int32(JZ_STATE, jz_tvalue val);
unsigned int jz_to_uint32(JZ_STATE, jz_tvalue val);

double jz_num_mod(JZ_STATE, jz_tvalue val1, jz_tvalue val2);
jz_str* jz_num_to_str(JZ_STATE, double num);

#endif
