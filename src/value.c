#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "value.h"
#include "string.h"
#include "object.h"
#include "num.h"

#define ABS(x)  ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) < 0 ? -1 : 1)

static void write_integral_double(UChar* buffer_end, double d);
static int add_decimal_point(UChar* buffer, int index);

jz_bool jz_values_equal(JZ_STATE, jz_val v1, jz_val v2) {
  if (JZ_VAL_TYPE(v1) == JZ_VAL_TYPE(v2) ||
      (JZ_IS_NUM(v1) && JZ_IS_NUM(v2)))
    return jz_values_strict_equal(jz, v1, v2);

  if (JZ_VAL_TYPE(v1) == jz_t_bool ||
      (JZ_VAL_TYPE(v1) == jz_t_str && JZ_IS_NUM(v2)))
    return jz_values_equal(jz, jz_to_wrapped_num(jz, v1), v2);

  if (JZ_VAL_TYPE(v2) == jz_t_bool ||
      (JZ_IS_NUM(v1) && JZ_VAL_TYPE(v2) == jz_t_str))
    return jz_values_equal(jz, v1, jz_to_wrapped_num(jz, v2));

  if (JZ_VAL_TYPE(v1) == jz_t_obj) {
    if (JZ_VAL_IS_NULL(v1))
      return JZ_VAL_TYPE(v2) == jz_t_undef;
    if (JZ_VAL_TYPE(v2) == jz_t_str || JZ_IS_NUM(v2))
      return jz_values_equal(jz, jz_to_primitive(jz, v1, jz_hint_none), v2);
    return jz_false;
  }

  if (JZ_VAL_TYPE(v2) == jz_t_obj) {
    if (JZ_VAL_IS_NULL(v2))
      return JZ_VAL_TYPE(v1) == jz_t_undef;
    if (JZ_VAL_TYPE(v1) == jz_t_str || JZ_IS_NUM(v1))
      return jz_values_equal(jz, v1, jz_to_primitive(jz, v2, jz_hint_none));
    return jz_false;
  }

  return jz_false;
}

jz_bool jz_values_strict_equal(JZ_STATE, jz_val v1, jz_val v2) {
  jz_type t1 = JZ_VAL_TYPE(v1);
  jz_type t2 = JZ_VAL_TYPE(v2);
  if (t1 != t2 && !(JZ_TYPE_IS_NUM(t1) && JZ_TYPE_IS_NUM(t2)))
    return jz_false;
  if (t1 == jz_t_str)
    return jz_str_equal(jz, jz_to_str(jz, v1), jz_to_str(jz, v2));
  if (JZ_VAL_IS_CONST(v1) || t1 == jz_t_obj) return v1 == v2;

  assert(JZ_TYPE_IS_NUM(t1));
  assert(JZ_TYPE_IS_NUM(t2));

  {
    double num1;
    double num2;
    jz_bool ret;

    num1 = jz_to_num(jz, v1);
    num2 = jz_to_num(jz, v2);

    ret = num1 == num2;
    return ret;
  }
}

double jz_values_comp(JZ_STATE, jz_val v1, jz_val v2) {
  double num1;
  double num2;

  v1 = jz_to_primitive(jz, v1, jz_hint_number);
  v2 = jz_to_primitive(jz, v2, jz_hint_number);

  if (JZ_VAL_TYPE(v1) == jz_t_str && JZ_VAL_TYPE(v2) == jz_t_str)
    return jz_str_comp(jz, jz_to_str(jz, v1), jz_to_str(jz, v2));

  num1 = jz_to_num(jz, v1);
  num2 = jz_to_num(jz, v2);

  if (JZ_NUM_IS_NAN(num1) || JZ_NUM_IS_NAN(num2))
    return JZ_NAN;

  if (JZ_NUM_IS_INF(num1) || JZ_NUM_IS_INF(num2)) {
    if (num1 == num2) return 0;
    else if (num1 < num2) return -1;
    else return 1;
  }

  return num1 - num2;
}

jz_val jz_wrap_int(JZ_STATE, int num) {
  if (num > JZ_INT_MAX || num < JZ_INT_MIN)
    return jz_wrap_num(jz, (double)num);

  return (jz_val)((num << 2) + jz_tt_int);
}

jz_val jz_wrap_num(JZ_STATE, double num) {
  jz_num* val;

  if (JZ_NUM_IS_INT(num) && !JZ_NUM_IS_NEG_0(num) &&
      num < JZ_INT_MAX && num > JZ_INT_MIN)
    return jz_wrap_int(jz, num);

  val = (jz_num*)jz_gc_malloc(jz, jz_t_num, sizeof(jz_num));
  val->num = num;
  return val;
}

double jz_to_num(JZ_STATE, jz_val val) {
  switch (JZ_VAL_TYPE(val)) {
  case jz_t_bool:
  case jz_t_int:   return (double)((int)(val) >> 2);
  case jz_t_num:   return ((jz_num*)val)->num;
  case jz_t_undef: return JZ_NAN;
  case jz_t_str:   return jz_str_to_num(jz, jz_to_str(jz, val));
  case jz_t_obj:
    if (JZ_VAL_IS_NULL(val))
      return 0;
    return jz_to_num(jz, jz_to_primitive(jz, val, jz_hint_number));
  default:
    fprintf(stderr, "[jz_to_num] Unknown jz_val type %d\n", JZ_VAL_TYPE(val));
    exit(1);
  }
}

jz_str* jz_to_str(JZ_STATE, jz_val val) {
  switch (JZ_VAL_TYPE(val)) {
  case jz_t_str: return (jz_str*)val;
  case jz_t_int:
  case jz_t_num: return jz_num_to_str(jz, jz_to_num(jz, val));
  case jz_t_bool:
    if ((jz_ival)(val) >> 2) return jz_str_from_literal(jz, "true");
    else return jz_str_from_literal(jz, "false");
  case jz_t_obj:
    if (JZ_VAL_IS_NULL(val))
      return jz_str_from_literal(jz, "null");
    return jz_to_str(jz, jz_to_primitive(jz, val, jz_hint_string));
  case jz_t_undef: return jz_str_from_literal(jz, "undefined");
  default:
    fprintf(stderr, "[jz_to_str] Unknown jz_val type %d\n", JZ_VAL_TYPE(val));
    exit(1);
  }
}

jz_obj* jz_to_obj(JZ_STATE, jz_val val) {
  switch (JZ_VAL_TYPE(val)) {
  case jz_t_obj:
    if (!JZ_VAL_IS_NULL(val))
      return (jz_obj*)val;
    fprintf(stderr, "TypeError: Can't convert `null' to object\n");
    exit(1);
  case jz_t_undef:
    fprintf(stderr, "TypeError: Can't convert `undefined' to object\n");
    exit(1);
  default:
    fprintf(stderr, "ToObject not implemented for jz_val type %d\n",
            JZ_VAL_TYPE(val));
    exit(1);
  }
}

jz_bool jz_to_bool(JZ_STATE, jz_val val) {
  switch (JZ_VAL_TYPE(val)) {
  case jz_t_bool:
  case jz_t_int: return (jz_bool)((int)(val) >> 2);
  case jz_t_num: {
    double num = jz_to_num(jz, val);

    if (JZ_NUM_IS_NAN(num)) return jz_false;
    else return (jz_bool)num;
  }
  case jz_t_str: return jz_to_str(jz, val)->length != 0;
  case jz_t_undef: return jz_false;
  case jz_t_obj: return !JZ_VAL_IS_NULL(val);
  default:
    fprintf(stderr, "[jz_to_bool] Unknown jz_val type %d\n", JZ_VAL_TYPE(val));
    exit(1);
  }
}

int jz_to_int32(JZ_STATE, jz_val val) {
  unsigned int num = jz_to_uint32(jz, val);
  if (num >= pow(2, 31)) return num - pow(2, 32);
  return num;
}

unsigned int jz_to_uint32(JZ_STATE, jz_val val) {
  double num = jz_to_num(jz, val);
  if (!((int)num) || JZ_NUM_IS_NAN(num) || JZ_NUM_IS_INF(num)) return 0;
  num = SIGN(num) * floor(ABS(num));

  {
    double divisor = pow(2, 32);
    /* return num % 2**32 */
    return num - divisor * floor(num / divisor);
  }
}


#define TRY_STRING_PRIMITIVE(val) {                  \
    jz_val res = jz_obj_to_str(jz, (jz_obj*)(val));  \
                                                     \
    if (JZ_VAL_IS_PRIMITIVE(res))                    \
      return res;                                    \
  }

#define TRY_VALUE_PRIMITIVE(val) {                           \
    jz_val res = jz_obj_value_of(jz, (jz_obj*)(val));        \
                                                             \
    if (JZ_VAL_IS_PRIMITIVE(res))                            \
      return res;                                            \
  }

jz_val jz_to_primitive(JZ_STATE, jz_val val,
                          jz_to_primitive_hint hint) {
  if (JZ_VAL_TYPE(val) != jz_t_obj || JZ_VAL_IS_NULL(val))
    return val;

  /* TODO: Really call [[DefaultValue]] */
  switch (hint) {
  case jz_hint_string:
    TRY_STRING_PRIMITIVE(val);
    TRY_VALUE_PRIMITIVE(val);
    break;
  case jz_hint_none:
  case jz_hint_number:
    TRY_VALUE_PRIMITIVE(val);
    TRY_STRING_PRIMITIVE(val);
    break;
  }
  fprintf(stderr, "Throw a TypeError here.\n");
  exit(1);
}

double jz_num_mod(JZ_STATE, jz_val val1, jz_val val2) {
  double dividend = jz_to_num(jz, val1);
  double divisor = jz_to_num(jz, val2);

  if (JZ_NUM_IS_NAN(dividend) || JZ_NUM_IS_NAN(divisor) ||
      JZ_NUM_IS_INF(dividend) || divisor == 0.0)
    return JZ_NAN;
  else if (JZ_NUM_IS_INF(divisor) || dividend == 0.0)
    return dividend;
  else if (SIGN(dividend) == SIGN(divisor))
    return dividend - divisor * floor(dividend / divisor);
  else
    return dividend - divisor * ceil(dividend / divisor);
}

#define DIGIT_CHAR(c)     ('0' + (int)(c))
#define DIGIT(num, place) (((num) / place) % 10)

#define FLOAT_SIG_FIGS 17

/* -X.XXXXXXXXXXXXXXXXe+XXX */
#define MAX_FLT_STR_LEN (FLOAT_SIG_FIGS + 7)

jz_str* jz_num_to_str(JZ_STATE, double num) {
  jz_str* to_ret;
  UChar* buffer;
  int order;
  jz_bool exponent;
  int length;

  if (JZ_NUM_IS_NAN(num)) return jz_str_from_literal(jz, "NaN");
  if (num == 0) return jz_str_from_literal(jz, "0");
  if (num < 0)
    return jz_str_concat(jz, jz_str_from_literal(jz, "-"),
                         jz_num_to_str(jz, -num));
  if (num == JZ_INF) return jz_str_from_literal(jz, "Infinity");

  to_ret = jz_str_alloc(jz, MAX_FLT_STR_LEN);
  buffer = JZ_STR_INT_PTR(to_ret);
  order = floor(log10(num));
  exponent = order > 20 || order < -6;

  write_integral_double(buffer + FLOAT_SIG_FIGS - 1,
                        num * pow(10, FLOAT_SIG_FIGS - order - 1));

  if (exponent) {
    UChar* buffer_top = buffer + add_decimal_point(buffer, 1);
    int abs_order = ABS(order);
    int hundreds = DIGIT(abs_order, 100);
    int tens = DIGIT(abs_order, 10);
    int ones = DIGIT(abs_order, 1);

    *buffer_top++ = 'e';
    *buffer_top++ = order > 0 ? '+' : '-';

    /* We rely here on the exponent never being greater than three digits. */
    if (hundreds != 0)
      *buffer_top++ = DIGIT_CHAR(hundreds);
    if (hundreds != 0 || tens != 0)
      *buffer_top++ = DIGIT_CHAR(tens);
    *buffer_top++ = DIGIT_CHAR(ones);

    length = buffer_top - buffer;
  }
  else length = add_decimal_point(buffer, order + 1);

  to_ret->length = length;
  return to_ret;
}

void write_integral_double(UChar* buffer_end, double d) {
  int i;

  assert(JZ_NUM_IS_INT(d));

  for (i = 0; i < FLOAT_SIG_FIGS; i++) {
    /* Is there a way to divide and mod at the same time here? */
    *buffer_end-- = DIGIT_CHAR(fmod(d, 10));
    d /= 10;
  }
}

int add_decimal_point(UChar* buffer, int index) {
  /* We may have extra dangling zeros if index < 0.
     MAX_FLT_STR_LEN isn't strictly accurate,
     but it's a good overestimate. */
  UChar temp_bottom[MAX_FLT_STR_LEN * 2];
  UChar* buffer_bottom = buffer;
  UChar* temp = temp_bottom;
  int i;

  /* Add "0." to the beginning if the index is 0 or negative */
  if (index < 1) {
    assert(FLOAT_SIG_FIGS + 2 - index < MAX_FLT_STR_LEN * 2);

    *temp++ = '0';
    *temp++ = '.';
  }

  /* If the index of the dot is negative,
     add 0s to pad it out. */
  for (; index < 0; index++) *temp++ = '0';

  /* We've already added the dot for these,
     so make index something that we won't pay attention to later on. */
  if (index == 0) index = -1;

  /* Copy from the integer string buffer,
     adding a decimal point where necessary. */
  for (i = 0; i < FLOAT_SIG_FIGS; i++) {
    if (i == index) *temp++ = '.';

    *temp++ = *buffer++;
  }
  temp--;

  /* Get rid of trailing 0s,
     and the decimal point if it's not necessary. */
  while (*temp == '0') *temp-- = '\0';
  if (*temp == '.') *temp-- = '\0';
  temp++;

  assert(temp - temp_bottom < MAX_FLT_STR_LEN);

  memcpy(buffer_bottom, temp_bottom, (temp - temp_bottom) * sizeof(UChar));
  return temp - temp_bottom;
}
