%{
#include "parse.h"
#include "lex.h"

#include <math.h>
#include <stdio.h>

jz_parse_node* root_node;

static void yyerror(const char* msg);
static jz_parse_node* node_new(jz_parse_type type, jz_parse_value car, jz_parse_value cdr);
%}

%error-verbose

%union {
  jz_str str;
  float num;
  char none;
}

%token <str> IDENTIFIER

%token <num> NUMBER

/* Punctuation tokens */
%token <none> LCURLY   RCURLY      LPAREN    RPAREN    LSQUARE      RSQUARE
              DOT      SEMICOLON   COMMA     LESS_THAN GREATER_THAN GT_EQ
              LT_EQ    EQ_EQ       NOT_EQ    EQ_EQ_EQ  NOT_EQ_EQ
              PLUS     MINUS       TIMES     MOD       PLUS_PLUS    MINUS_MINUS
              LT_LT    GT_GT       GT_GT_GT  BW_AND    BW_OR        XOR
              NOT      BW_NOT      AND       OR        QUESTION     COLON
              EQUALS   PLUS_EQ     MINUS_EQ  TIMES_EQ  MOD_EQ       LT_LT_EQ
              GT_GT_EQ GT_GT_GT_EQ BW_AND_EQ BW_OR_EQ  XOR_EQ
%token <none> DIV DIV_EQ

%type <none> expression
%type <num> additive_expression multiplicative_expression

%start expression

%%

expression: additive_expression {
  root_node = node_new(jz_parse_identifier,
                       (jz_parse_value){.node = NULL},
                       (jz_parse_value){.node = NULL});
  printf("Result: %f\n", $1);
 };

additive_expression: multiplicative_expression { $$ = $1; }
  | additive_expression PLUS  multiplicative_expression { $$ = $1 + $3; }
  | additive_expression MINUS multiplicative_expression { $$ = $1 - $3; };

multiplicative_expression: NUMBER { $$ = $1; }
  | multiplicative_expression TIMES NUMBER { $$ = $1 * $3; }
  | multiplicative_expression DIV   NUMBER { $$ = $1 / $3; };
  | multiplicative_expression MOD   NUMBER {
    if (($1 > 0 && $3 > 0) || ($1 < 0 && $3 < 0)) $$ = $1 - $3 * floor($1 / $3);
    else $$ = $1 - $3 * ceil($1 / $3);
 };

%%

jz_parse_node* node_new(jz_parse_type type, jz_parse_value car, jz_parse_value cdr) {
  jz_parse_node* to_ret = malloc(sizeof(jz_parse_node));
  to_ret->type = type;
  to_ret->car  = car;
  to_ret->cdr  = cdr;
  return to_ret;
}

jz_parse_node* jz_parse_string(jz_str code) {
  jz_lex_set_code(code);
  if(!yyparse()) return NULL;
  return root_node;
}

void yyerror(const char* msg)
{
  fprintf(stderr, "%s\n", msg);
}
