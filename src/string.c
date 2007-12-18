#include "string.h"

jz_str jz_str_new(int length, UChar* value) {
  jz_str to_ret;
  to_ret.length = length;
  to_ret.value = value;
  return to_ret;
}
