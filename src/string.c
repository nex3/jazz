#include "string.h"

#include <stdlib.h>
#include <string.h>

jz_str* jz_str_new(int length, const UChar* value) {
  jz_str* to_ret = malloc(sizeof(jz_str));
  to_ret->length = length;
  to_ret->value = value;
  return to_ret;
}

jz_str* jz_str_deep_new(int length, const UChar* value) {
  jz_str* to_ret;
  UChar* buffer;

  if (length == 0) return jz_str_null();

  to_ret = malloc(sizeof(jz_str));
  to_ret->length = length;

  buffer = calloc(sizeof(UChar), length);
  memcpy(buffer, value, length * sizeof(UChar));
  to_ret->value = buffer;

  return to_ret;
}

jz_str* jz_str_null() {
  return jz_str_new(0, NULL);
}

jz_str* jz_str_dup(const jz_str* this) {
  return jz_str_new(this->length, this->value);
}

jz_str* jz_str_deep_dup(const jz_str* this) {
  return jz_str_deep_new(this->length, this->value);
}

jz_str* jz_str_substr(const jz_str* this, int start, int end) {
  if (end <= start) return jz_str_null();
  return jz_str_new(end - start, this->value + start);
}

bool jz_str_equal(const jz_str* s1, const jz_str* s2) {
  if (s1->length != s2->length) return false;

  return u_strncmp(s1->value, s2->value, s1->length) == 0;
}

char* jz_str_to_chars(const jz_str* this) {
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
