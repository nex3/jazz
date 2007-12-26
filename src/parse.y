%{
#include "parse.h"
#include "lex.h"

#include <stdbool.h>
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
static jz_parse_node* unop_node(jz_op_type type, jz_parse_node* next);
%}

%error-verbose

%union {
  struct jz_parse_node* node;
  jz_str str;
  double num;
  char boolean;
  char none;
}

%token <str> IDENTIFIER

%token <num> NUMBER

/* Constant Literal Tokens */
%token <none> TRUE_VAL FALSE_VAL UNDEF_VAL

/* Keyword Tokens */
%token <none> RETURN

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

%type <node> program source_elements
%type <node> statement expr_statement return_statement empty_statement

%type <node> expr cond_expr or_expr and_expr bw_or_expr bw_and_expr xor_expr
             eq_expr rel_expr shift_expr add_expr mult_expr

%type <node> unary_expr primary_expr

%type <node> literal number boolean undefined
%type <boolean> bool_val

%start program

%%

program: source_elements {
  $$ = $1;
  root_node = $$;
 }

source_elements: statement {
  DECLARE_UNIONS(node, $1, node, NULL);
  $$ = node_new(jz_parse_statements, car, cdr);
 }
  | source_elements statement {
    DECLARE_UNIONS(node, $2, node, $1);
    $$ = node_new(jz_parse_statements, car, cdr);
 }

statement: expr_statement { $$ = $1; }
  | empty_statement { $$ = $1; }
  | return_statement { $$ = $1; }

expr_statement: expr SEMICOLON {
  DECLARE_UNIONS(st_type, jz_st_expr, node, $1);
  $$ = node_new(jz_parse_statement, car, cdr);
 }

return_statement: RETURN expr SEMICOLON {
  DECLARE_UNIONS(st_type, jz_st_return, node, $2);
  $$ = node_new(jz_parse_statement, car, cdr);
 }
  | RETURN SEMICOLON {
    DECLARE_UNIONS(st_type, jz_st_return, node, NULL);
    $$ = node_new(jz_parse_statement, car, cdr);
 }

empty_statement: SEMICOLON {
  DECLARE_UNIONS(st_type, jz_st_empty, node, NULL);
  $$ = node_new(jz_parse_statement, car, cdr);
 }

expr: cond_expr { $$ = $1; }

cond_expr: or_expr { $$ = $1; }
  | or_expr QUESTION expr COLON expr {
    jz_parse_node* cont;

    {
      DECLARE_UNIONS(node, $3, node, $5);
      cont = node_new(jz_parse_cont, car, cdr);
    }
    {
      DECLARE_UNIONS(node, $1, node, cont);
      cont = node_new(jz_parse_cont, car, cdr);
    }
    {
      DECLARE_UNIONS(op_type, jz_op_cond, node, cont);
      $$ = node_new(jz_parse_triop, car, cdr);
    }
 }

or_expr: and_expr { $$ = $1; }
  | or_expr OR and_expr { $$ = binop_node(jz_op_or, $1, $3); }

and_expr: bw_or_expr { $$ = $1; }
  | and_expr AND bw_or_expr { $$ = binop_node(jz_op_and, $1, $3); }

bw_or_expr: xor_expr { $$ = $1; }
  | bw_or_expr BW_OR xor_expr { $$ = binop_node(jz_op_bw_or, $1, $3); }

xor_expr: bw_and_expr { $$ = $1; }
  | xor_expr XOR bw_and_expr { $$ = binop_node(jz_op_xor, $1, $3); }

bw_and_expr: eq_expr { $$ = $1; }
  | bw_and_expr BW_AND eq_expr { $$ = binop_node(jz_op_bw_and, $1, $3); }

eq_expr: rel_expr { $$ = $1; }
  | eq_expr EQ_EQ     rel_expr { $$ = binop_node(jz_op_equals,    $1, $3); }
  | eq_expr EQ_EQ_EQ  rel_expr { $$ = binop_node(jz_op_eq_eq_eq,  $1, $3); }
  | eq_expr NOT_EQ    rel_expr {
    $$ = binop_node(jz_op_equals, $1, $3);
    $$ = unop_node(jz_op_not, $$);
 }
  | eq_expr NOT_EQ_EQ rel_expr {
    $$ = binop_node(jz_op_eq_eq_eq, $1, $3);
    $$ = unop_node(jz_op_not, $$);
 }

rel_expr: shift_expr { $$ = $1; }
  | rel_expr LESS_THAN    shift_expr { $$ = binop_node(jz_op_lt,    $1, $3); }
  | rel_expr GREATER_THAN shift_expr { $$ = binop_node(jz_op_gt,    $1, $3); }
  | rel_expr LT_EQ        shift_expr { $$ = binop_node(jz_op_lt_eq, $1, $3); }
  | rel_expr GT_EQ        shift_expr { $$ = binop_node(jz_op_gt_eq, $1, $3); }

shift_expr: add_expr { $$ = $1; }
  | shift_expr LT_LT    add_expr { $$ = binop_node(jz_op_lt_lt,    $1, $3); }
  | shift_expr GT_GT    add_expr { $$ = binop_node(jz_op_gt_gt,    $1, $3); }
  | shift_expr GT_GT_GT add_expr { $$ = binop_node(jz_op_gt_gt_gt, $1, $3); }

add_expr: mult_expr { $$ = $1; }
  | add_expr PLUS  mult_expr { $$ = binop_node(jz_op_plus,  $1, $3); }
  | add_expr MINUS mult_expr { $$ = binop_node(jz_op_minus, $1, $3); };

mult_expr: unary_expr { $$ = $1; }
  | mult_expr TIMES unary_expr { $$ = binop_node(jz_op_times, $1, $3); }
  | mult_expr DIV   unary_expr { $$ = binop_node(jz_op_div,   $1, $3); }
  | mult_expr MOD   unary_expr { $$ = binop_node(jz_op_mod,   $1, $3); };

unary_expr: primary_expr { $$ = $1; }
  | PLUS   unary_expr { $$ = unop_node(jz_op_plus,   $2); }
  | MINUS  unary_expr { $$ = unop_node(jz_op_minus,  $2); }
  | BW_NOT unary_expr { $$ = unop_node(jz_op_bw_not, $2); }
  | NOT    unary_expr { $$ = unop_node(jz_op_not,    $2); }

primary_expr: literal { $$ = $1; }
  | LPAREN expr RPAREN { $$ = $2; }

literal: number { $$ = $1; }
  | boolean     { $$ = $1; }
  | undefined   { $$ = $1; }

number: NUMBER {
  DECLARE_UNIONS(val, jz_wrap_num($1), node, NULL);
  $$ = node_new(jz_parse_literal, car, cdr);
 };

boolean: bool_val {
  DECLARE_UNIONS(val, jz_wrap_bool($1), node, NULL);
  $$ = node_new(jz_parse_literal, car, cdr);
 };

undefined: UNDEF_VAL {
  DECLARE_UNIONS(val, jz_undef_val(), node, NULL);
  $$ = node_new(jz_parse_literal, car, cdr);
 }

bool_val: TRUE_VAL { $$ = true; }
  | FALSE_VAL { $$ = false; }

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
    DECLARE_UNIONS(op_type, type, node, cont);
    return node_new(jz_parse_binop, car, cdr);
  }
}

jz_parse_node* unop_node(jz_op_type type, jz_parse_node* next) {
  DECLARE_UNIONS(op_type, type, node, next);
  return node_new(jz_parse_unop, car, cdr);
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
