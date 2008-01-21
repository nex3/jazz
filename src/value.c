#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"
#include "string.h"

#define ABS(x)  ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) < 0 ? -1 : 1)

static void write_integral_double(UChar* buffer_end, double d);
static int add_decimal_point(UChar* buffer, int index);

bool jz_values_equal(JZ_STATE, jz_tvalue v1, jz_tvalue v2) {
  if (JZ_TVAL_TYPE(v1) == JZ_TVAL_TYPE(v2))
    return jz_values_strict_equal(jz, v1, v2);

  if (JZ_TVAL_TYPE(v1) == jz_bool ||
      (JZ_TVAL_TYPE(v1) == jz_strt && JZ_TVAL_TYPE(v2) == jz_num))
    return jz_values_equal(jz, jz_to_wrapped_num(jz, v1), v2);

  if (JZ_TVAL_TYPE(v2) == jz_bool ||
      (JZ_TVAL_TYPE(v1) == jz_num && JZ_TVAL_TYPE(v2) == jz_strt))
    return jz_values_equal(jz, v1, jz_to_wrapped_num(jz, v2));

  return false;
}

bool jz_values_strict_equal(JZ_STATE, jz_tvalue v1, jz_tvalue v2) {
  if (JZ_TVAL_TYPE(v1) != JZ_TVAL_TYPE(v2)) return false;
  if (JZ_TVAL_TYPE(v1) == jz_strt)
    return jz_str_equal(jz, v1.value.str, v2.value.str);
  if (JZ_TVAL_TYPE(v1) == jz_bool) return v1.value.b == v2.value.b;
  if (JZ_TVAL_TYPE(v1) == jz_undef) return true;
  else {
    double num1 = v1.value.num;
    double num2 = v2.value.num;
    
    assert(JZ_TVAL_TYPE(v1) == jz_num);
    return num1 == num2;
  }
}

double jz_values_comp(JZ_STATE, jz_tvalue v1, jz_tvalue v2) {
  double num1;
  double num2;

  if (JZ_TVAL_TYPE(v1) == jz_strt && JZ_TVAL_TYPE(v2) == jz_strt)
    return jz_str_comp(jz, v1.value.str, v2.value.str);

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

jz_tvalue jz_wrap_num(JZ_STATE, double num) {
  jz_tvalue tvalue;
  JZ_TVAL_SET_TYPE(tvalue, jz_num);
  tvalue.value.num = num;
  return tvalue;
}

jz_tvalue jz_wrap_str(JZ_STATE, jz_str* str) {
  jz_tvalue tvalue;
  JZ_TVAL_SET_TYPE(tvalue, jz_strt);
  tvalue.value.str = str;
  return tvalue;
}

jz_tvalue jz_wrap_bool(JZ_STATE, bool b) {
  jz_tvalue tvalue;
  JZ_TVAL_SET_TYPE(tvalue, jz_bool);
  tvalue.value.b = b;
  return tvalue;
}

double jz_to_num(JZ_STATE, jz_tvalue val) {
  switch (JZ_TVAL_TYPE(val)) {
  case jz_num:   return val.value.num;
  case jz_bool:  return (double)(val.value.b);
  case jz_undef: return JZ_NAN;
  case jz_strt:  return jz_str_to_num(jz, val.value.str);
  default:
    fprintf(stderr, "Unknown jz_tvalue type %d\n", JZ_TVAL_TYPE(val));
    exit(1);
  }
}

/* Note: this function allocates new values,
   but only sometimes.
   It's a source of memory leaks until we get GC going. */
jz_str* jz_to_str(JZ_STATE, jz_tvalue val) {
  switch (JZ_TVAL_TYPE(val)) {
  case jz_strt: return val.value.str;
  case jz_num: return jz_num_to_str(jz, val.value.num);
  case jz_bool:
    if (val.value.b) return jz_str_from_literal(jz, "true");
    else return jz_str_from_literal(jz, "false");
  case jz_undef: return jz_str_from_literal(jz, "undefined");
  default:
    fprintf(stderr, "Unknown jz_tvalue type %d\n", JZ_TVAL_TYPE(val));
    exit(1);
  }
}

bool jz_to_bool(JZ_STATE, jz_tvalue val) {
  switch (JZ_TVAL_TYPE(val)) {
  case jz_bool: return val.value.b;
  case jz_num:
    if (JZ_NUM_IS_NAN(val.value.num)) return false;
    else return (bool)(val.value.num);
  case jz_strt: return val.value.str->length != 0;
  case jz_undef: return false;
  default:
    fprintf(stderr, "Unknown jz_tvalue type %d\n", JZ_TVAL_TYPE(val));
    exit(1);
  }
}

int jz_to_int32(JZ_STATE, jz_tvalue val) {
  unsigned int num = jz_to_uint32(jz, val);
  if (num >= pow(2, 31)) return num - pow(2, 32);
  return num;
}

unsigned int jz_to_uint32(JZ_STATE, jz_tvalue val) {
  double num = jz_to_num(jz, val);
  if (!((int)num) || JZ_NUM_IS_NAN(num) || JZ_NUM_IS_INF(num)) return 0;
  num = SIGN(num) * floor(ABS(num));

  {
    double divisor = pow(2, 32);
    /* return num % 2**32 */
    return num - divisor * floor(num / divisor);
  }
}

double jz_num_mod(JZ_STATE, jz_tvalue val1, jz_tvalue val2) {
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
  bool exponent;
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
