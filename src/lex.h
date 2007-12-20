#ifndef JZ_LEX_H
#define JZ_LEX_H

#include <unicode/ustring.h>
#include "string.h"

void jz_lex_init();
void jz_lex_set_code(jz_str code);

int yylex();

#endif
