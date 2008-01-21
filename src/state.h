#ifndef JZ_STATE_H
#define JZ_STATE_H

#include <unicode/uregex.h>
#include "type.h"

typedef struct {
  jz_tvalue undefined_val;
  jz_tvalue true_val;
  jz_tvalue false_val;
  struct {
    URegularExpression* identifier_re;
    URegularExpression* whitespace_re;
    URegularExpression* line_terminator_re;
    URegularExpression* punctuation_re;
    URegularExpression* hex_literal_re;
    URegularExpression* decimal_literal_re1;
    URegularExpression* decimal_literal_re2;
    URegularExpression* decimal_literal_re3;
    URegularExpression* string_literal_re;
  } lex;
} jz_state;

#define JZ_STATE jz_state* jz

#define JZ_UNDEFINED (jz->undefined_val)
#define JZ_TRUE      (jz->true_val)
#define JZ_FALSE     (jz->false_val)

#define JZ_TVAL_TYPE(value)           ((value).tag & 0x03)
#define JZ_TVAL_SET_TYPE(value, type) ((value).tag = ((value).tag & !0x03) | type)

jz_state* jz_init();

#endif
