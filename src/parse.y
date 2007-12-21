%{
#include "parse.h"
#include "lex.h"

#include <stdio.h>

static jz_parse_node* root_node = NULL;

#define DECLARE_UNIONS(car_member, car_val, cdr_member, cdr_val) \
  jz_parse_value car;                                            \
  jz_parse_value cdr;                                            \
  car.car_member = car_val;                                      \
  cdr.cdr_member = cdr_val;

static void yyerror(const char* msg);
static jz_parse_node* node_new(jz_parse_type type, jz_parse_value car, jz_parse_value cdr);

static jz_parse_node* binop_node(jz_op_type type, jz_parse_node* left, jz_parse_node* right);
%}

%error-verbose

%union {
  struct jz_parse_node* node;
  jz_str str;
  double num;
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

%type <node> expression additive_expression multiplicative_expression
%type <node> number

%start expression

%%

expression: additive_expression {
  $$ = $1;
  root_node = $$;
 };

additive_expression: multiplicative_expression { $$ = $1; }
  | additive_expression PLUS  multiplicative_expression { $$ = binop_node(jz_op_plus,  $1, $3); }
  | additive_expression MINUS multiplicative_expression { $$ = binop_node(jz_op_minus, $1, $3); };

multiplicative_expression: number { $$ = $1; }
  | multiplicative_expression TIMES number { $$ = binop_node(jz_op_times, $1, $3); }
  | multiplicative_expression DIV   number { $$ = binop_node(jz_op_div,   $1, $3); }
  | multiplicative_expression MOD   number { $$ = binop_node(jz_op_mod,   $1, $3); };

number: NUMBER {
  DECLARE_UNIONS(num, $1, node, NULL);
  $$ = node_new(jz_parse_num, car, cdr);
 };

%%

jz_parse_node* node_new(jz_parse_type type, jz_parse_value car, jz_parse_value cdr) {
  jz_parse_node* to_ret = malloc(sizeof(jz_parse_node));
  to_ret->type = type;
  to_ret->car  = car;
  to_ret->cdr  = cdr;
  return to_ret;
}

jz_parse_node* binop_node(jz_op_type type, jz_parse_node* left, jz_parse_node* right) {
  jz_parse_node* cont;

  {
    DECLARE_UNIONS(node, left, node, right);
    cont = node_new(jz_parse_cont, car, cdr);
  }
  {
    DECLARE_UNIONS(op_type, jz_op_mod, node, cont);
    return node_new(jz_parse_binop, car, cdr);
  }
}

jz_parse_node* jz_parse_string(jz_str code) {
  jz_lex_set_code(code);

  /* yyparse returns 0 to indicate success. */
  if(yyparse()) return NULL;

  return root_node;
}

void yyerror(const char* msg)
{
  fprintf(stderr, "%s\n", msg);
}
