#ifndef JZ_STRING_H
#define JZ_STRING_H

#include <unicode/ustring.h>

typedef struct {
  int length;
  UChar* value;
} jz_str;

jz_str jz_str_new(int length, UChar* value);
jz_str jz_str_null();
jz_str jz_str_substr(jz_str this, int start, int length);
char*  jz_str_to_chars(jz_str this);

#endif
