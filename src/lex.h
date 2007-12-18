#ifndef JZ_LEX_H
#define JZ_LEX_H

#include <unicode/ustring.h>
#include <unicode/uregex.h>
#include <stdbool.h>
#include "string.h"

typedef enum {
  jz_token_eoi = 0,
  jz_token_keyword,
  jz_token_identifier
} jz_token_type;

bool jz_lex_init(jz_str code);

jz_token_type yylex();

#endif
