/* The Jazz lexer.
   This is mainly used by the parser,
   but must be initialized before parsing can begin.
   See jz_lex_init and jz_lex_set_code. */

#ifndef JZ_LEX_H
#define JZ_LEX_H

#include <unicode/ustring.h>
#include "string.h"
#include "y.tab.h"
#include "parse.h"

typedef struct {
  jz_str* code;
  jz_str* code_prev;
} jz_lex_state;

/* Initializes the lexer to lex from the given string. */
jz_lex_state* jz_lex_init(JZ_STATE, const jz_str* code);

/* The yacc parser interface function. */
int yylex(YYSTYPE* lex_val, JZ_STATE, jz_lex_state* state);


double jz_parse_number(JZ_STATE, const jz_str* num);

#endif
