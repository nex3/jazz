#include "object.h"

jz_tvalue jz_wrap_num(double num) {
  jz_tvalue tvalue;
  tvalue.type = jz_num;
  tvalue.value.num = num;
  return tvalue;
}
