#include "type.h"

#include <math.h>
#include <assert.h>

#define ABS(x)  ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) < 0 ? -1 : 1)

bool jz_values_equal(jz_tvalue v1, jz_tvalue v2) {
  if (v1.type == v2.type) {
    if (v1.type == jz_bool) return v1.value.b == v2.value.b;
    else {
      double num1 = v1.value.num;
      double num2 = v2.value.num;

      assert(v1.type == jz_num);
      /* If both are positive or negative zero, return true */
      if (!((int)num1) && !((int)num2)) return true;
      return num1 == num2;
    }
  }

  if (v1.type == jz_bool) return jz_values_equal(jz_wrap_num(jz_to_num(v1)), v2);
  if (v2.type == jz_bool) return jz_values_equal(v1, jz_wrap_num(jz_to_num(v2)));
  else return false;
}

jz_tvalue jz_wrap_num(double num) {
  jz_tvalue tvalue;
  tvalue.type = jz_num;
  tvalue.value.num = num;
  return tvalue;
}

jz_tvalue jz_wrap_bool(bool b) {
  jz_tvalue tvalue;
  tvalue.type = jz_bool;
  tvalue.value.b = b;
  return tvalue;
}

double jz_to_num(jz_tvalue val) {
  switch (val.type) {
  case jz_bool: return (double)(val.value.b);
  case jz_num:  return val.value.num;
  default: assert(0);
  }
}

bool jz_to_bool(jz_tvalue val) {
  switch (val.type) {
  case jz_bool: return val.value.b;
  case jz_num:
    if (JZ_NUM_IS_NAN(val.value.num)) return false;
    else return (bool)(val.value.num);
  default: assert(0);
  }
}

int jz_to_int32(jz_tvalue val) {
  double num = jz_to_num(val);
  if (!((int)num) || JZ_NUM_IS_NAN(num) || JZ_NUM_IS_INF(num)) return 0;
  num = SIGN(num) * floor(ABS(num));
  /* num = num % 2**32 */
  num = num - num * floor(num / pow(2.0, 32.0));
  if (num >= pow(2.0, 31.0)) num *= -1;
  return (int)num;
}
