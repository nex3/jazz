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

#define SET_HASHED(str) (JZ_GC_UTAG_AND((str), 2))

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

/* This is based heavily on Bob Jenkins' universal hash code,
   available in the public domain at
   http://www.burtleburtle.net/bob/hash/doobs.html.

   TODO: Make this use jz_uint32 when it exists. */

/* If we really want to optimize this,
   we could put the literal table values in here,
   but that would bloat the code.
   Worth testing later on. */
#define TWIDDLE_HASH(ch, hash, table) {         \
    if (ch & 0x01)   hash ^= table[i + 0];      \
    if (ch & 0x02)   hash ^= table[i + 1];      \
    if (ch & 0x04)   hash ^= table[i + 2];      \
    if (ch & 0x08)   hash ^= table[i + 3];      \
    if (ch & 0x10)   hash ^= table[i + 4];      \
    if (ch & 0x20)   hash ^= table[i + 5];      \
    if (ch & 0x40)   hash ^= table[i + 6];      \
    if (ch & 0x80)   hash ^= table[i + 7];      \
    if (ch & 0x100)  hash ^= table[i + 0];      \
    if (ch & 0x200)  hash ^= table[i + 1];      \
    if (ch & 0x400)  hash ^= table[i + 2];      \
    if (ch & 0x800)  hash ^= table[i + 3];      \
    if (ch & 0x1000) hash ^= table[i + 4];      \
    if (ch & 0x2000) hash ^= table[i + 5];      \
    if (ch & 0x4000) hash ^= table[i + 6];      \
    if (ch & 0x8000) hash ^= table[i + 7];      \
  }

const unsigned int uhash_table1[16] = {
  0x1b8f81ba, 0x35bf02eb, 0x320f8b96, 0xf002bcd6, 0x697a01cd, 0x1558ebcc,
  0x83b0ed10, 0x5fe7ca65, 0x64ed00d4, 0x688a2bd, 0x515b4bd0, 0xa8b27d90,
  0x1c4278d6, 0xe9fd6745, 0x4e42dd21, 0xcb012ef8
};

const unsigned int uhash_table2[16] = {
  0x31470ff7, 0xb2ff6c3e, 0xc5ca2ba9, 0x93f84faa, 0x82148f0d, 0x458bf77c,
  0x53ec7216, 0xbf34800e, 0x42144502, 0xa3653def, 0xc0372859, 0xf466a9bd,
  0x5e6b0f5f, 0x20f8199b, 0x1162070a, 0x6fb2d2f4
};

void jz_str_compute_hashes(JZ_STATE, jz_str* this) {
  int length = this->length;
  unsigned int hash1 = length;
  unsigned int hash2 = length;
  const UChar* str = JZ_STR_PTR(this);
  ptrdiff_t i;

  if (JZ_STR_IS_HASHED(this))
    return;

  for (i = 0; i < (length << 3); i += 8)
  {
    register UChar ch = str[i >> 3];
    TWIDDLE_HASH(ch, hash1, uhash_table1);
    TWIDDLE_HASH(ch, hash2, uhash_table2);
  }

  this->hash1 = hash1;
  this->hash2 = hash2;
}
