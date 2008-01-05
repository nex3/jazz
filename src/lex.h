/* The Jazz lexer.
   This is mainly used by the parser,
   but must be initialized before parsing can begin.
   See jz_lex_init and jz_lex_set_code. */

#ifndef JZ_LEX_H
#define JZ_LEX_H

#include <unicode/ustring.h>
#include "string.h"

/* Initializes the lexer.
   This must be called before parsing can begin,
   and before jz_lex_set_code is called. */
void jz_lex_init();

/* Sets the string from which the lexer will read tokens.
   This must be called before parsing can begin. */
void jz_lex_set_code(const jz_str* code);


/* The yacc parser interface function. */
int yylex();

#endif
