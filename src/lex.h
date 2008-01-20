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

/* Initializes new lexer state. */
jz_lex_state* jz_lex_init();

/* Sets the string from which the lexer will read tokens.
   This must be called before parsing can begin. */
void jz_lex_set_code(jz_lex_state* state, const jz_str* code);


/* The yacc parser interface function. */
int yylex(YYSTYPE* lex_val, jz_lex_state* state);


double jz_parse_number(const jz_str* num);

#endif
