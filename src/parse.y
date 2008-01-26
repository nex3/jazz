%{
#include "parse.h"
#include "lex.h"
#include "state.h"
#include "string.h"
#include "object.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

/* Reverses an s-expression list and returns the new list.
   The list is destructively modified,
   but no nodes are allocated or freed. */
static jz_parse_node* reverse_list(JZ_STATE, jz_parse_node* head);


/* The ptr_to functions allocate pointers to value objects,
   so they can be used as members of jz_parse_val. */
static jz_tvalue* ptr_to_val(JZ_STATE, jz_tvalue val);
static jz_op_type* ptr_to_ot(JZ_STATE, jz_op_type ot);

static void yyerror(JZ_STATE, jz_parse_node** root, jz_lex_state* state, const char* msg);

#define binop_node(jz, type, left, right) jz_pnode_list(jz, jz_parse_binop, 3, ptr_to_ot(jz, type), (left), (right))
#define unop_node(jz, type, next) jz_pnode_cons(jz, jz_parse_unop, ptr_to_ot(jz, type), (next))
%}

%error-verbose

%pure-parser
%parse-param {jz_state* jz}
%parse-param {jz_parse_node** root}
%parse-param {jz_lex_state* state}
%lex-param   {jz_state* jz}
%lex-param   {jz_lex_state* state}

/* We have one shift/reduce conflict due to an intentional ambiguity
   in the ECMAscript grammar.
   "if (foo) if (bar) baz; else boom;" could be parsed as
   "if (foo) { if (bar) baz; } else boom;" or
   "if (foo) { if (bar) baz; else boom; }".
   The spec mandates the former as the actual interpretation,
   and that's what Bison defaults to,
   so we mark the conflict as expected. */
%expect 1

%union {
  struct jz_parse_node* node;
  jz_str* str;
  double num;
  char boolean;
  char none;
}

/* Literal value tokens. */
%token <str> IDENTIFIER STRING
%token <num> NUMBER

/* Constant Literal Tokens */
%token <none> TRUE_VAL FALSE_VAL NULL_VAL UNDEF_VAL NAN_VAL INF_VAL

/* Keyword Tokens */
%token <none> RETURN VAR IF ELSE DO WHILE FOR SWITCH CASE DEFAULT

/* Punctuation tokens */
%token <none> LCURLY    RCURLY      LPAREN    RPAREN    LSQUARE      RSQUARE
              DOT       SEMICOLON   COMMA     LESS_THAN GREATER_THAN GT_EQ
              LT_EQ     EQ_EQ       NOT_EQ    STRICT_EQ NOT_STRICT_EQ
              PLUS      MINUS       TIMES     MOD       PLUS_PLUS    MINUS_MINUS
              LSHIFT    RSHIFT      URSHIFT   BW_AND    BW_OR        XOR
              NOT       BW_NOT      AND       OR        QUESTION     COLON
              EQUALS    PLUS_EQ     MINUS_EQ  TIMES_EQ  MOD_EQ       LSHIFT_EQ
              RSHIFT_EQ URSHIFT_EQ  BW_AND_EQ BW_OR_EQ  XOR_EQ
%token <none> DIV DIV_EQ

/* Grammmar Productions */
%type <node> program source_elements source_element_list source_element
             statement block statements statement_list var_statement
             var_decls var_decl_list var_decl
             expr_statement return_statement empty_statement if_statement else
             iter_statement do_while_statement while_statement for_statement
             opt_expr switch_statement case_block case_block_list case_clauses
             case_clause default_clause expr expr_list
             assign_expr cond_expr or_expr and_expr bw_or_expr xor_expr
             bw_and_expr eq_expr rel_expr shift_expr add_expr mult_expr
             unary_expr postfix_expr left_hand_expr new_expr member_expr
             primary_expr identifier literal number string boolean undefined
             not_a_number infinity null

%type <boolean> bool_val

%start program

%%

program: source_elements { *root = $1; }

source_elements: source_element_list { $$ = reverse_list(jz, $1); }

source_element_list: source_element { $$ = jz_pnode_wrap(jz, jz_parse_statements, $1); }
  | source_element_list source_element { $$ = jz_pnode_cons(jz, jz_parse_statements, $2, $1); }

source_element: statement { $$ = $1; }

statement: block     { $$ = $1; }
  | expr_statement   { $$ = $1; }
  | var_statement    { $$ = $1; }
  | empty_statement  { $$ = $1; }
  | return_statement { $$ = $1; }
  | if_statement     { $$ = $1; }
  | iter_statement   { $$ = $1; }
  | switch_statement { $$ = $1; }

block: LCURLY statements RCURLY { $$ = $2; }
  | LCURLY RCURLY { $$ = jz_pnode_new(jz, jz_parse_empty); }

statements: statement_list { $$ = reverse_list(jz, $1); }

statement_list: statement { $$ = jz_pnode_wrap(jz, jz_parse_statements, $1); }
  | statement_list statement { $$ = jz_pnode_cons(jz, jz_parse_statements, $2, $1); }

var_statement: VAR var_decls SEMICOLON { $$ = $2; }

var_decls: var_decl_list { $$ = reverse_list(jz, $1); }

var_decl_list: var_decl { $$ = jz_pnode_wrap(jz, jz_parse_vars, $1); }
  | var_decl_list COMMA var_decl { $$ = jz_pnode_cons(jz, jz_parse_vars, $3, $1); }

var_decl: IDENTIFIER {
  $$ = jz_pnode_wrap(jz, jz_parse_var, jz_str_deep_dup(jz, $1));
 }
  | IDENTIFIER EQUALS assign_expr {
    $$ = jz_pnode_cons(jz, jz_parse_var, jz_str_deep_dup(jz, $1), $3);
 }

expr_statement: expr SEMICOLON { $$ = $1; }

return_statement: RETURN expr SEMICOLON { $$ = jz_pnode_wrap(jz, jz_parse_return, $2); }
  | RETURN SEMICOLON { $$ = jz_pnode_wrap(jz, jz_parse_return, NULL); }

empty_statement: SEMICOLON { $$ = jz_pnode_new(jz, jz_parse_empty); }

if_statement: IF LPAREN expr RPAREN statement else {
  $$ = jz_pnode_list(jz, jz_parse_if, 3, $3, $5, $6);
 }

else: ELSE statement { $$ = $2; }
  | /* empty */ { $$ = NULL; }

iter_statement: do_while_statement { $$ = $1; }
  | while_statement { $$ = $1; }
  | for_statement { $$ = $1; }

do_while_statement: DO statement WHILE LPAREN expr RPAREN SEMICOLON {
  $$ = jz_pnode_cons(jz, jz_parse_do_while, $5, $2);
 }

while_statement: WHILE LPAREN expr RPAREN statement {
  $$ = jz_pnode_cons(jz, jz_parse_while, $3, $5);
 }

for_statement
  : FOR LPAREN opt_expr SEMICOLON
    opt_expr SEMICOLON opt_expr RPAREN statement {
    $$ = jz_pnode_list(jz, jz_parse_for, 4, $3, $5, $7, $9);
 }
  | FOR LPAREN VAR var_decls SEMICOLON
    opt_expr SEMICOLON opt_expr RPAREN statement {
    $$ = jz_pnode_list(jz, jz_parse_for, 4, $4, $6, $8, $10);
 }  

opt_expr: expr { $$ = $1; }
  | /* empty */ { $$ = NULL; }

switch_statement: SWITCH LPAREN expr RPAREN LCURLY case_block RCURLY {
  $$ = jz_pnode_cons(jz, jz_parse_switch, $3, $6);
 }

case_block: case_block_list { $$ = reverse_list(jz, $1); }

case_block_list: case_clauses { $$ = $1; }
  | default_clause { $$ = $1; }
  | case_clauses default_clause { $$ = jz_plist_concat(jz, $2, $1); }
  | default_clause case_clauses { $$ = jz_plist_concat(jz, $2, $1); }
  | case_clauses default_clause case_clauses { $$ = jz_plist_concat(jz, jz_plist_concat(jz, $3, $2), $1); }
  | /* empty */ { $$ = NULL; }

case_clauses: case_clause { $$ = jz_pnode_wrap(jz, jz_parse_cases, $1); }
  | case_clauses case_clause { $$ = jz_pnode_cons(jz, jz_parse_cases, $2, $1); }

case_clause: CASE expr COLON statement_list { $$ = jz_pnode_cons(jz, jz_parse_case, $2, $4); }
  | CASE expr COLON { $$ = jz_pnode_wrap(jz, jz_parse_case, $2); }

default_clause: DEFAULT COLON statement_list { $$ = jz_pnode_wrap(jz, jz_parse_cases, jz_pnode_cons(jz, jz_parse_case, NULL, $3)); }
| DEFAULT COLON { $$ = jz_pnode_wrap(jz, jz_parse_cases, jz_pnode_new(jz, jz_parse_case)); }

expr: expr_list { $$ = reverse_list(jz, $1); }

expr_list: assign_expr { $$ = jz_pnode_wrap(jz, jz_parse_exprs, $1); }
  | expr_list COMMA assign_expr {
    $$ = jz_pnode_cons(jz, jz_parse_exprs, $3, $1);
 }

assign_expr: cond_expr { $$ = $1; }
  | left_hand_expr EQUALS     assign_expr
     { $$ = binop_node(jz, jz_op_assign,     $1, $3); }
  | left_hand_expr TIMES_EQ   assign_expr
     { $$ = binop_node(jz, jz_op_times_eq,   $1, $3); }
  | left_hand_expr DIV_EQ     assign_expr
     { $$ = binop_node(jz, jz_op_div_eq,     $1, $3); }
  | left_hand_expr MOD_EQ     assign_expr
     { $$ = binop_node(jz, jz_op_mod_eq,     $1, $3); }
  | left_hand_expr PLUS_EQ    assign_expr
     { $$ = binop_node(jz, jz_op_add_eq,     $1, $3); }
  | left_hand_expr MINUS_EQ   assign_expr
     { $$ = binop_node(jz, jz_op_sub_eq,     $1, $3); }
  | left_hand_expr LSHIFT_EQ  assign_expr
     { $$ = binop_node(jz, jz_op_lshift_eq,  $1, $3); }
  | left_hand_expr RSHIFT_EQ  assign_expr
     { $$ = binop_node(jz, jz_op_rshift_eq,  $1, $3); }
  | left_hand_expr URSHIFT_EQ assign_expr
     { $$ = binop_node(jz, jz_op_urshift_eq, $1, $3); }
  | left_hand_expr BW_AND_EQ  assign_expr
     { $$ = binop_node(jz, jz_op_bw_and_eq,  $1, $3); }
  | left_hand_expr XOR_EQ     assign_expr
     { $$ = binop_node(jz, jz_op_xor_eq,     $1, $3); }
  | left_hand_expr BW_OR_EQ   assign_expr
     { $$ = binop_node(jz, jz_op_bw_or_eq,   $1, $3); }

cond_expr: or_expr { $$ = $1; }
  | or_expr QUESTION assign_expr COLON assign_expr {
    $$ = jz_pnode_list(jz, jz_parse_triop, 4, ptr_to_ot(jz, jz_op_cond), $1, $3, $5);
 }

or_expr: and_expr { $$ = $1; }
  | or_expr OR and_expr { $$ = binop_node(jz, jz_op_or, $1, $3); }

and_expr: bw_or_expr { $$ = $1; }
  | and_expr AND bw_or_expr { $$ = binop_node(jz, jz_op_and, $1, $3); }

bw_or_expr: xor_expr { $$ = $1; }
  | bw_or_expr BW_OR xor_expr { $$ = binop_node(jz, jz_op_bw_or, $1, $3); }

xor_expr: bw_and_expr { $$ = $1; }
  | xor_expr XOR bw_and_expr { $$ = binop_node(jz, jz_op_xor, $1, $3); }

bw_and_expr: eq_expr { $$ = $1; }
  | bw_and_expr BW_AND eq_expr { $$ = binop_node(jz, jz_op_bw_and, $1, $3); }

eq_expr: rel_expr { $$ = $1; }
  | eq_expr EQ_EQ     rel_expr { $$ = binop_node(jz, jz_op_equals,    $1, $3); }
  | eq_expr STRICT_EQ rel_expr { $$ = binop_node(jz, jz_op_strict_eq, $1, $3); }
  | eq_expr NOT_EQ    rel_expr {
    $$ = unop_node(jz, jz_op_not, binop_node(jz, jz_op_equals, $1, $3));
 }
  | eq_expr NOT_STRICT_EQ rel_expr {
    $$ = unop_node(jz, jz_op_not, binop_node(jz, jz_op_strict_eq, $1, $3));
 }

rel_expr: shift_expr { $$ = $1; }
  | rel_expr LESS_THAN    shift_expr { $$ = binop_node(jz, jz_op_lt,    $1, $3); }
  | rel_expr GREATER_THAN shift_expr { $$ = binop_node(jz, jz_op_gt,    $1, $3); }
  | rel_expr LT_EQ        shift_expr { $$ = binop_node(jz, jz_op_lt_eq, $1, $3); }
  | rel_expr GT_EQ        shift_expr { $$ = binop_node(jz, jz_op_gt_eq, $1, $3); }

shift_expr: add_expr { $$ = $1; }
  | shift_expr LSHIFT  add_expr { $$ = binop_node(jz, jz_op_lshift,  $1, $3); }
  | shift_expr RSHIFT  add_expr { $$ = binop_node(jz, jz_op_rshift,  $1, $3); }
  | shift_expr URSHIFT add_expr { $$ = binop_node(jz, jz_op_urshift, $1, $3); }

add_expr: mult_expr { $$ = $1; }
  | add_expr PLUS  mult_expr { $$ = binop_node(jz, jz_op_add, $1, $3); }
  | add_expr MINUS mult_expr { $$ = binop_node(jz, jz_op_sub, $1, $3); };

mult_expr: unary_expr { $$ = $1; }
  | mult_expr TIMES unary_expr { $$ = binop_node(jz, jz_op_times, $1, $3); }
  | mult_expr DIV   unary_expr { $$ = binop_node(jz, jz_op_div,   $1, $3); }
  | mult_expr MOD   unary_expr { $$ = binop_node(jz, jz_op_mod,   $1, $3); };

unary_expr: postfix_expr { $$ = $1; }
  | PLUS        unary_expr { $$ = unop_node(jz, jz_op_add,     $2); }
  | MINUS       unary_expr { $$ = unop_node(jz, jz_op_sub,     $2); }
  | BW_NOT      unary_expr { $$ = unop_node(jz, jz_op_bw_not,  $2); }
  | NOT         unary_expr { $$ = unop_node(jz, jz_op_not,     $2); }
  | PLUS_PLUS   unary_expr { $$ = unop_node(jz, jz_op_pre_inc, $2); }
  | MINUS_MINUS unary_expr { $$ = unop_node(jz, jz_op_pre_dec, $2); }

postfix_expr: left_hand_expr   { $$ = $1; }
  | left_hand_expr PLUS_PLUS   { $$ = unop_node(jz, jz_op_post_inc, $1); }
  | left_hand_expr MINUS_MINUS { $$ = unop_node(jz, jz_op_post_dec, $1); }

left_hand_expr: new_expr { $$ = $1; }
new_expr: member_expr { $$ = $1; }
member_expr: primary_expr { $$ = $1; }

primary_expr: identifier { $$ = $1; }
  | literal { $$ = $1; }
  | LPAREN expr RPAREN { $$ = $2; }

identifier: IDENTIFIER {
  $$ = jz_pnode_wrap(jz, jz_parse_identifier, jz_str_deep_dup(jz, $1));
 }

literal: number  { $$ = $1; }
  | string       { $$ = $1; }
  | boolean      { $$ = $1; }
  | undefined    { $$ = $1; }
  | not_a_number { $$ = $1; }
  | infinity     { $$ = $1; }
  | null         { $$ = $1; }

number: NUMBER {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, ptr_to_val(jz, jz_wrap_num(jz, $1)));
 }

string: STRING {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, ptr_to_val(jz, jz_wrap_str(jz, $1)));
 }

boolean: bool_val {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, ptr_to_val(jz, jz_wrap_bool(jz, $1)));
 };

bool_val: TRUE_VAL { $$ = true; }
  | FALSE_VAL { $$ = false; }

undefined: UNDEF_VAL {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, ptr_to_val(jz, JZ_UNDEFINED));
 }

not_a_number: NAN_VAL {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, ptr_to_val(jz, jz_wrap_num(jz, JZ_NAN)));
 }

infinity: INF_VAL {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, ptr_to_val(jz, jz_wrap_num(jz, JZ_INF)));
 }

null: NULL_VAL {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, ptr_to_val(jz, JZ_NULL));
 }

%%

jz_parse_node* jz_pnode_list(JZ_STATE, jz_parse_type type, int argc, ...) {
  jz_parse_node* to_ret = jz_pnode_new(jz, type);
  jz_parse_node* end = to_ret;
  va_list args;
  int i;

  va_start(args, argc);

  to_ret->type = type;
  to_ret->car  = va_arg(args, jz_parse_value);

  for (i = 2; i < argc; i++) {
    jz_parse_node* next = jz_pnode_new(jz, jz_parse_cont);

    end->cdr.node = next;
    end = next;
    end->car = va_arg(args, jz_parse_value);
  }

  if (argc > 1) end->cdr = va_arg(args, jz_parse_value);
  else end->cdr.node = NULL;

  va_end(args);
  return to_ret;
}

jz_parse_node* jz_pnode_new(JZ_STATE, jz_parse_type type) {
  jz_parse_node* to_ret = malloc(sizeof(jz_parse_node));;
  to_ret->type = type;
  to_ret->car.node = to_ret->cdr.node = NULL;
  return to_ret;
}

jz_parse_node* jz_plist_concat(JZ_STATE, jz_parse_node* list1, jz_parse_node* list2) {
  jz_parse_node* next = list1;

  if (list1 == NULL) return list2;

  while (next->cdr.node != NULL) next = next->cdr.node;
  next->cdr.node = list2;

  return list1;
}

jz_parse_node* jz_parse_string(JZ_STATE, const jz_str* code) {
  jz_parse_node* root = NULL;
  jz_lex_state* state = jz_lex_init(jz, code);
  int result = yyparse(jz, &root, state);

  free(state);

  /* yyparse returns 0 to indicate success. */
  return result == 0 ? root : NULL;
}

jz_parse_node* reverse_list(JZ_STATE, jz_parse_node* head) {
  jz_parse_node *prev = NULL, *curr = NULL, *next = NULL;

  if (head == NULL) return NULL;

  curr = head;
  next = curr->cdr.node;

  while (next != NULL) {
    curr->cdr.node = prev;

    prev = curr;
    curr = next;
    next = next->cdr.node;
  }

  curr->cdr.node = prev;

  return curr;
}

jz_tvalue* ptr_to_val(JZ_STATE, jz_tvalue val) {
  jz_tvalue* to_ret = malloc(sizeof(jz_tvalue));
  *to_ret = val;
  return to_ret;
}

jz_op_type* ptr_to_ot(JZ_STATE, jz_op_type ot) {
  jz_op_type* to_ret = malloc(sizeof(jz_op_type));
  *to_ret = ot;
  return to_ret;
}

void yyerror(JZ_STATE, jz_parse_node** root, jz_lex_state* state, const char* msg)
{
  fprintf(stderr, "%s\n", msg);
}
