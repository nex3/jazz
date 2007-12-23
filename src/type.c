#include "type.h"

#include <math.h>
#include <assert.h>

#define ABS(x)  ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) < 0 ? -1 : 1)

bool jz_values_equal(jz_tvalue v1, jz_tvalue v2) {
  if (v1.type == v2.type) return jz_values_strict_equal(v1, v2);
  if (v1.type == jz_bool) return jz_values_equal(jz_wrap_num(jz_to_num(v1)), v2);
  if (v2.type == jz_bool) return jz_values_equal(v1, jz_wrap_num(jz_to_num(v2)));
  else return false;
}

bool jz_values_strict_equal(jz_tvalue v1, jz_tvalue v2) {
  if (v1.type != v2.type) return false;
  if (v1.type == jz_bool) return v1.value.b == v2.value.b;
  else {
    double num1 = v1.value.num;
    double num2 = v2.value.num;
    
    assert(v1.type == jz_num);   
    return num1 == num2;
  }
}

double jz_values_comp(jz_tvalue v1, jz_tvalue v2) {
  double num1 = jz_to_num(v1);
  double num2 = jz_to_num(v2);

  if (JZ_NUM_IS_NAN(num1) || JZ_NUM_IS_NAN(num2))
    return num1;

  if (JZ_NUM_IS_INF(num1) || JZ_NUM_IS_INF(num2)) {
    if (num1 == num2) return 0;
    else if (num1 < num2) return -1;
    else return 1;
  }

  return num1 - num2;
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
  int num = jz_to_uint32(val);
  if (num >= pow(2, 31)) num *= -1;
  return num;
}

unsigned int jz_to_uint32(jz_tvalue val) {
  double num = jz_to_num(val);
  if (!((int)num) || JZ_NUM_IS_NAN(num) || JZ_NUM_IS_INF(num)) return 0;
  num = SIGN(num) * floor(ABS(num));
  /* return num % 2**32 */
  return (int)(num - num * floor(num / pow(2.0, 32.0)));
}
