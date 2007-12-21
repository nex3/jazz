#include "string.h"
#include <stdlib.h>

jz_str jz_str_new(int length, UChar* value) {
  jz_str to_ret;
  to_ret.length = length;
  to_ret.value = value;
  return to_ret;
}

jz_str jz_str_substr(jz_str this, int start, int length) {
  return jz_str_new(length, this.value + start);
}

char* jz_str_to_chars(jz_str this) {
  UErrorCode error = U_ZERO_ERROR;
  char* to_ret = calloc(this.length + 1, sizeof(char));
  u_strToUTF8(to_ret, this.length, NULL, this.value, this.length, &error);
  to_ret[this.length] = '\0';
  return to_ret;
}
