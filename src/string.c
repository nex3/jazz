#include "string.h"
#include "lex.h"
#include "gc.h"
#include "state.h"

#include <unicode/uchar.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define SET_EXT(str) (JZ_GC_UTAG_OR((str), 1))
#define SET_INT(str) (JZ_GC_UTAG_AND((str), ~1))

#define SET_HASHED(str) (JZ_GC_UTAG_OR((str), 2))

static bool is_whitespace_char(UChar c);
static jz_str* str_new(JZ_STATE, int start, int length);
static jz_str_value* val_alloc(JZ_STATE, int length);

bool is_whitespace_char(UChar c) {
  return u_isblank(c) || (c) == 0xA0 || (c) == '\f' || (c) == '\v' ||
    (c) == '\r' || (c) == '\n' || (c) == 0x2028 || (c) == 0x2029;
}

jz_str* str_new(JZ_STATE, int start, int length) {
  jz_str* to_ret = (jz_str*)jz_gc_malloc(jz, jz_t_str, sizeof(jz_str));
  to_ret->start = start;
  to_ret->length = length;
  return to_ret;
}

jz_str* str_val_new(JZ_STATE, int start, int length, jz_str_value* val) {
  jz_str* to_ret = str_new(jz, start, length);

  JZ_GC_WRITE_BARRIER(jz, val, to_ret);
  to_ret->value.val = val;
  return to_ret;
}

jz_str_value* val_alloc(JZ_STATE, int length) {
  jz_str_value* to_ret;

  if (length == 0)
    return NULL;

  to_ret = (jz_str_value*)jz_gc_dyn_malloc(jz, jz_t_str_value,
                                           sizeof(jz_str_value),
                                           sizeof(UChar), length);
  return to_ret;
}

jz_str* jz_str_external(JZ_STATE, int length, const UChar* value) {
  jz_str* to_ret = str_new(jz, 0, length);

  SET_EXT(to_ret);
  to_ret->value.ext = value;
  return to_ret;
}

jz_str* jz_str_deep_new(JZ_STATE, int length, const UChar* value) {
  jz_str* to_ret;
  jz_str_value* buffer;

  if (length == 0)
    return jz_str_null(jz);

  to_ret = str_new(jz, 0, length);
  buffer = val_alloc(jz, length);
  memcpy(buffer->str, value, length * sizeof(UChar));
  to_ret->value.val = buffer;

  return to_ret;
}

jz_str* jz_str_alloc(JZ_STATE, int space) {
  jz_str_value* val = val_alloc(jz, space);
  return str_val_new(jz, 0, 0, val);
}

jz_str* jz_str_null(JZ_STATE) {
  return jz_str_external(jz, 0, NULL);
}

jz_str* jz_str_dup(JZ_STATE, const jz_str* this) {
  jz_str* to_ret = (jz_str*)jz_gc_malloc(jz, jz_t_str, sizeof(jz_str));
  JZ_GC_SET_UTAG(to_ret, JZ_GC_UTAG(this));
  to_ret->start = this->start;
  to_ret->length = this->length;
  to_ret->value = this->value;
  return to_ret;
}

jz_str* jz_str_deep_dup(JZ_STATE, const jz_str* this) {
  return jz_str_deep_new(jz, this->length, JZ_STR_PTR(this));
}

jz_str* jz_str_substr(JZ_STATE, const jz_str* this, int start) {
  return jz_str_substr2(jz, this, start, this->length);
}

jz_str* jz_str_substr2(JZ_STATE, const jz_str* this, int start, int end) {
  jz_str* to_ret;

  if (end <= start) return jz_str_null(jz);

  to_ret = str_new(jz, this->start + start, end - start);
  if (JZ_STR_IS_EXT(this)) SET_EXT(to_ret);
  to_ret->value = this->value;
  return to_ret;
}

jz_str* jz_str_strip(JZ_STATE, const jz_str* this) {
  const UChar* bottom = JZ_STR_PTR(this);
  const UChar* start = bottom;
  const UChar* end = start + this->length - 1;

  if (start == NULL) return jz_str_null(jz);

  for (; start - bottom < this->length && is_whitespace_char(*start); start++);
  if (start - bottom == this->length)
    return jz_str_null(jz);

  for (; is_whitespace_char(*end); end--);

  return jz_str_substr2(jz, this, start - bottom, end - bottom + 1);
}

jz_str* jz_str_concat(JZ_STATE, const jz_str* s1, const jz_str* s2) {
  jz_str_value* val = val_alloc(jz, s1->length + s2->length);
  UChar* buffer = val->str;
  const UChar* s1_ptr = JZ_STR_PTR(s1);
  const UChar* s2_ptr = JZ_STR_PTR(s2); 
  int i;

  if (s1->length == 0) return jz_str_dup(jz, s2);
  else if (s2->length == 0) return jz_str_dup(jz, s1);

  for (i = 0; i < s1->length; i++) *buffer++ = s1_ptr[i];
  for (i = 0; i < s2->length; i++) *buffer++ = s2_ptr[i];

  return str_val_new(jz, 0, s1->length + s2->length, val);
}

bool jz_str_equal(JZ_STATE, const jz_str* s1, const jz_str* s2) {
  if (s1->length != s2->length) return false;

  return u_strncmp(JZ_STR_PTR(s1), JZ_STR_PTR(s2), s1->length) == 0;
}

int jz_str_comp(JZ_STATE, const jz_str* s1, const jz_str* s2) {
  const UChar* s1_bottom = JZ_STR_PTR(s1);
  const UChar* s1_iter = s1_bottom;
  const UChar* s2_bottom = JZ_STR_PTR(s2);
  const UChar* s2_iter = s2_bottom;

  for (;; s1_iter++, s2_iter++) {
    if (s1_iter - s1_bottom == s1->length) {
      if (s2_iter - s2_bottom == s2->length) return 0;
      else return -1;
    }
    if (s2_iter - s2_bottom == s2->length) return 1;

    if (*s1_iter < *s2_iter) return -1;
    else if (*s1_iter > *s2_iter) return 1;
  }

  fprintf(stderr, "There is a bug in the jz_str_comp "
          "when comparing \"%s\" and \"%s\". Returning 0.\n",
          jz_str_to_chars(jz, s1), jz_str_to_chars(jz, s2));
  return 0;
}

double jz_str_to_num(JZ_STATE, const jz_str* num) {
  jz_str* my_num = jz_str_strip(jz, num);
  const UChar* num_ptr = JZ_STR_PTR(my_num);
  char sign = 1;
  double to_ret;

  if (my_num->length == 0) return 0;

  if (*num_ptr == '-') {
    sign = -1;
    my_num = jz_str_substr(jz, my_num, 1);
  } else if (*num_ptr == '+') {
    sign = 1;
    my_num = jz_str_substr(jz, my_num, 1);
  }

  to_ret = jz_parse_number(jz, my_num);

  /* If the string couldn't be parsed,
     it might be "Infinity" */
  if (JZ_NUM_IS_NAN(to_ret)) {
    jz_str* val = jz_str_from_chars(jz, "Infinity", 8);

    if (jz_str_equal(jz, my_num, val)) to_ret = JZ_INF;
  }

  return sign * to_ret;
}

char* jz_str_to_chars(JZ_STATE, const jz_str* this) {
  char* to_ret;
  UErrorCode error = U_ZERO_ERROR;

  if (this == NULL) {
    to_ret = malloc(sizeof(char));
    to_ret[0] = '\0';
    return to_ret;
  }

  to_ret = calloc(sizeof(char), this->length + 1);
  u_strToUTF8(to_ret, this->length, NULL, JZ_STR_PTR(this), this->length, &error);
  to_ret[this->length] = '\0';
  return to_ret;
}

jz_str* jz_str_from_chars(JZ_STATE, const char* value, int length) {
  jz_str* to_ret = jz_str_alloc(jz, length);
  UErrorCode error = U_ZERO_ERROR;

  u_strFromUTF8(JZ_STR_INT_PTR(to_ret), length, &to_ret->length,
                value, length, &error);

  if (U_FAILURE(error)) {
    fprintf(stderr, "ICU Error: %s\n", u_errorName(error));
    exit(1);
  }

  return to_ret;
}

/* This is based on Paul Hsieh's SuperFastHash.
   See http://www.azillionmonkeys.com/qed/hash.html.

   TODO: uint32 all around. */
unsigned int jz_str_hash(JZ_STATE, jz_str* this) {
  unsigned int len = this->length, hash = this->length, tmp;
  const UChar* data = JZ_STR_PTR(this);

  if (len == 0)
    return 0;

  if (JZ_STR_IS_HASHED(this))
    return this->hash;

  len >>= 1;

  for (; len > 0; len--) {
    hash  += data[0];
    tmp    = (data[1] << 11) ^ hash;
    hash   = (hash << 16) ^ tmp;
    data  += 2;
    hash  += hash >> 11;
  }

  /* Handle odd last character */
  if (len & 1) {
    hash += data[0];
    hash ^= hash << 11;
    hash += hash >> 17;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  this->hash = hash;
  SET_HASHED(this);

  return hash;
}
