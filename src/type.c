#include "type.h"

#include <assert.h>

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
