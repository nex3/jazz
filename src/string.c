#include "string.h"
#include <stdlib.h>

jz_str* jz_str_new(int length, UChar* value) {
  jz_str* to_ret = malloc(sizeof(jz_str));
  to_ret->length = length;
  to_ret->value = value;
  return to_ret;
}

jz_str* jz_str_null() {
  return jz_str_new(0, NULL);
}

jz_str* jz_str_dup(jz_str* this) {
  return jz_str_new(this->length, this->value);
}

jz_str* jz_str_substr(jz_str* this, int start, int length) {
  return jz_str_new(length, this->value + start);
}

bool jz_str_equal(const jz_str* s1, const jz_str* s2) {
  if (s1->length != s2->length) return false;

  return u_strncmp(s1->value, s2->value, s1->length) == 0;
}

char* jz_str_to_chars(jz_str* this) {
  char* to_ret;
  UErrorCode error = U_ZERO_ERROR;

  if (this == NULL) {
    to_ret = malloc(sizeof(char));
    to_ret[0] = '\0';
    return to_ret;
  }

  to_ret = calloc(this->length + 1, sizeof(char));
  u_strToUTF8(to_ret, this->length, NULL, this->value, this->length, &error);
  to_ret[this->length] = '\0';
  return to_ret;
}
