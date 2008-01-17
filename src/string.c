#include "string.h"
#include "lex.h"
#include "type.h"

#include <unicode/uchar.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

#define IS_WHITESPACE_CHAR(c)                                           \
  (u_isblank(c) || (c) == 0xA0 || (c) == '\f' || (c) == '\v' ||         \
   (c) == '\r' || (c) == '\n' || (c) == 0x2028 || (c) == 0x2029)

jz_str* jz_str_strip(const jz_str* this) {
  const UChar* start = this->value;
  const UChar* end = this->value + this->length - 1;

  if (start == NULL) return this;

  while (true) {
    UChar ch;
    if (start - this->value == this->length) return jz_str_null();

    ch = *start;
    if (!IS_WHITESPACE_CHAR(ch)) break;
    start++;
  }

  while (true) {
    UChar ch = *end;
    if (!IS_WHITESPACE_CHAR(ch)) break;
    end--;
  }

  return jz_str_new(end - start + 1, start);
}

jz_str* jz_str_concat(const jz_str* s1, const jz_str* s2) {
  UChar* buffer = calloc(sizeof(UChar), s1->length + s2->length);
  UChar* buffer_bottom = buffer;
  int i;

  for (i = 0; i < s1->length; i++) *buffer++ = s1->value[i];
  for (i = 0; i < s2->length; i++) *buffer++ = s2->value[i];

  return jz_str_new(s1->length + s2->length, buffer_bottom);
}

bool jz_str_equal(const jz_str* s1, const jz_str* s2) {
  if (s1->length != s2->length) return false;

  return u_strncmp(s1->value, s2->value, s1->length) == 0;
}

int jz_str_comp(const jz_str* s1, const jz_str* s2) {
  const UChar* s1_iter = s1->value;
  const UChar* s2_iter = s2->value;

  for (;; s1_iter++, s2_iter++) {
    if (s1_iter - s1->value == s1->length) {
      if (s2_iter - s2->value == s2->length) return 0;
      else return -1;
    }
    if (s2_iter - s2->value == s2->length) return 1;

    if (*s1_iter < *s2_iter) return -1;
    else if (*s1_iter > *s2_iter) return 1;
  }

  fprintf(stderr, "There is a bug in the jz_str_comp "
          "when comparing \"%s\" and \"%s\". Returning 0.\n",
          jz_str_to_chars(s1), jz_str_to_chars(s2));
  return 0;
}

double jz_str_to_num(const jz_str* num) {
  jz_str* my_num = jz_str_strip(num);
  char sign = 1;
  double to_ret;

  if (my_num->value == NULL) return 0;

  if (*my_num->value == '-') {
    sign = -1;
    my_num->value++;
    my_num->length--;
  } else if (*my_num->value == '+') {
    sign = 1;
    my_num->value++;
    my_num->length--;
  }
  
  to_ret = jz_parse_number(my_num);

  /* If the string couldn't be parsed,
     it might be "Infinity" */
  if (JZ_NUM_IS_NAN(to_ret)) {
    JZ_STR_DECLARE(val, "Infinity");

    if (jz_str_equal(my_num, val)) to_ret = JZ_INF;
  }

  free(my_num);

  return sign * to_ret;
}

char* jz_str_to_chars(const jz_str* this) {
  char* to_ret;
  UErrorCode error = U_ZERO_ERROR;

  if (this == NULL) {
    to_ret = malloc(sizeof(char));
    to_ret[0] = '\0';
    return to_ret;
  }

  to_ret = calloc(sizeof(char), this->length + 1);
  u_strToUTF8(to_ret, this->length, NULL, this->value, this->length, &error);
  to_ret[this->length] = '\0';
  return to_ret;
}

jz_str* jz_str_from_chars(const char* value, int length) {
  UChar* buffer = calloc(sizeof(UChar), length);
  jz_str* to_ret = malloc(sizeof(jz_str));
  UErrorCode error = U_ZERO_ERROR;

  u_strFromUTF8(buffer, length, &to_ret->length,
                value, length, &error);

  if (U_FAILURE(error)) {
    fprintf(stderr, "ICU Error: %s\n", u_errorName(error));
    exit(1);
  }

  to_ret->value = buffer;
  return to_ret;
}
