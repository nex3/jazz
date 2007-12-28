#ifndef JZ_STRING_H
#define JZ_STRING_H

#include <unicode/ustring.h>

#include <stdbool.h>

typedef struct {
  int length;
  UChar* value;
} jz_str;

jz_str* jz_str_new(int length, UChar* value);
jz_str* jz_str_dup(jz_str* this);
jz_str* jz_str_substr(jz_str* this, int start, int length);
bool    jz_str_equal(const jz_str* s1, const jz_str* s2);
char*   jz_str_to_chars(jz_str* this);

#endif
