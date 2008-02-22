%{
#include "parse.h"
#include "lex.h"
#include "state.h"
#include "string.h"
#include "object.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

/* Reverses an s-expression list and returns the new list.
   The list is destructively modified,
   but no nodes are allocated or freed. */
static jz_parse_node* reverse_list(JZ_STATE, jz_parse_node* head);

static void print_parse_list(JZ_STATE, jz_parse_node* node, jz_bool start);
static void print_parse_ptr(JZ_STATE, jz_parse_ptr ptr);

static void free_parse_ptr(JZ_STATE, jz_parse_ptr ptr);

static void yyerror(JZ_STATE, jz_parse_node** root, jz_lex_state* state, const char* msg);

#define CONS(car, cdr) jz_pnode_cons(jz, (jz_tag*)(car), (jz_tag*)(cdr))

#define binop_node(jz, type, left, right) jz_pnode_list(jz, 4, jz_pleaf_new(jz, jz_parse_binop), jz_pleaf_new(jz, type), (left), (right))
#define unop_node(jz, type, next) jz_pnode_pair(jz, jz_parse_unop, jz_pleaf_new(jz, type), (next))
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
  int operation;
  jz_tvalue tvalue;
  char none;
}

/* Literal value tokens. */
%token <str> IDENTIFIER STRING
%token <num> NUMBER

/* Constant Literal Tokens */
%token <none> TRUE_VAL FALSE_VAL NULL_VAL

/* Keyword Tokens */
%token <none> THIS RETURN VAR IF ELSE DO WHILE FOR SWITCH CASE DEFAULT FUNCTION

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
             iter_statement do_while_statement while_statement for_statement first_for_expr
             opt_expr switch_statement case_block case_block_list case_clauses case_clause default_clause
                  expr      expr_list      assign_expr      cond_expr
             stmt_expr stmt_expr_list stmt_assign_expr stmt_cond_expr
                  or_expr      and_expr      bw_or_expr      xor_expr
             stmt_or_expr stmt_and_expr stmt_bw_or_expr stmt_xor_expr
                  bw_and_expr      eq_expr      rel_expr      shift_expr
             stmt_bw_and_expr stmt_eq_expr stmt_rel_expr stmt_shift_expr
                  add_expr      mult_expr      unary_expr
             stmt_add_expr stmt_mult_expr stmt_unary_expr
                  postfix_expr      left_hand_expr      new_expr
             stmt_postfix_expr stmt_left_hand_expr stmt_new_expr
                  call_expr      call_or_member_expr      member_expr      primary_expr
             stmt_call_expr stmt_call_or_member_expr stmt_member_expr stmt_primary_expr
             function_expr opt_source_elements arguments argument_list member_accessor
             identifier literal object_literal

%type <operation> assign_expr_op eq_expr_op neq_expr_op rel_expr_op shift_expr_op
                  add_expr_op mult_expr_op unary_expr_op postfix_expr_op

%type <tvalue> literal_tval

%type <boolean> bool_val

%start program

%%

program: source_elements { *root = $1; }

source_elements: source_element_list { $$ = reverse_list(jz, $1); }

source_element_list: source_element { $$ = CONS($1, NULL); }
  | source_element_list source_element { $$ = CONS($2, $1); }

source_element: statement

statement: block | expr_statement | var_statement | empty_statement
  | return_statement | if_statement | iter_statement  | switch_statement

block: LCURLY statements RCURLY { $$ = CONS(jz_pleaf_new(jz, jz_parse_block), $2); }
  | LCURLY RCURLY { $$ = NULL; }

statements: statement_list { $$ = reverse_list(jz, $1); }

statement_list: statement { $$ = CONS($1, NULL); }
  | statement_list statement { $$ = CONS($2, $1); }

var_statement: VAR var_decls SEMICOLON { $$ = $2; }

var_decls: var_decl_list { $$ = CONS(jz_pleaf_new(jz, jz_parse_var), reverse_list(jz, $1)); }

var_decl_list: var_decl { $$ = CONS($1, NULL); }
  | var_decl_list COMMA var_decl { $$ = CONS($3, $1); }

var_decl: IDENTIFIER {
  $$ = jz_pnode_list(jz, 2, jz_str_deep_dup(jz, $1), NULL);
 }
  | IDENTIFIER EQUALS assign_expr {
    $$ = jz_pnode_list(jz, 2, jz_str_deep_dup(jz, $1), $3);
 }

expr_statement: stmt_expr SEMICOLON

return_statement: RETURN expr SEMICOLON { $$ = jz_pnode_wrap(jz, jz_parse_return, $2); }
  | RETURN SEMICOLON { $$ = jz_pnode_wrap(jz, jz_parse_return, NULL); }

empty_statement: SEMICOLON { $$ = NULL; }

if_statement: IF LPAREN expr RPAREN statement else {
   $$ = jz_pnode_list(jz, 4, jz_pleaf_new(jz, jz_parse_if), $3, $5, $6);
 }

else: ELSE statement { $$ = $2; }
  | /* empty */ { $$ = NULL; }

iter_statement: do_while_statement | while_statement | for_statement

do_while_statement: DO statement WHILE LPAREN expr RPAREN SEMICOLON {
  $$ = jz_pnode_pair(jz, jz_parse_do_while, $5, $2);
 }

while_statement: WHILE LPAREN expr RPAREN statement {
  $$ = jz_pnode_pair(jz, jz_parse_while, $3, $5);
 }

for_statement: FOR LPAREN first_for_expr SEMICOLON
    opt_expr SEMICOLON opt_expr RPAREN statement {
      $$ = jz_pnode_list(jz, 5, jz_pleaf_new(jz, jz_parse_for), $3, $5, $7, $9);
 }

first_for_expr: opt_expr
  | VAR var_decls { $$ = $2; }

opt_expr: expr
  | /* empty */ { $$ = NULL; }


switch_statement: SWITCH LPAREN expr RPAREN LCURLY case_block RCURLY {
  $$ = CONS(jz_pleaf_new(jz, jz_parse_switch), CONS($3, $6));
 }

case_block: case_block_list { $$ = reverse_list(jz, $1); }

case_block_list: case_clauses | default_clause
  | case_clauses default_clause { $$ = jz_plist_concat(jz, $2, $1); }
  | default_clause case_clauses { $$ = jz_plist_concat(jz, $2, $1); }
  | case_clauses default_clause case_clauses { $$ = jz_plist_concat(jz, jz_plist_concat(jz, $3, $2), $1); }
  | /* empty */ { $$ = NULL; }

case_clauses: case_clause { $$ = CONS($1, NULL); }
  | case_clauses case_clause { $$ = CONS($2, $1); }

case_clause: CASE expr COLON statement_list { $$ = CONS($2, $4); }
  | CASE expr COLON { $$ = CONS($2, NULL); }

default_clause: DEFAULT COLON statement_list { $$ = CONS(CONS(NULL, $3), NULL); }
  | DEFAULT COLON { $$ = CONS(CONS(NULL, NULL), NULL); }

expr: expr_list { $$ = CONS(jz_pleaf_new(jz, jz_parse_expr), reverse_list(jz, $1)); }
stmt_expr: stmt_expr_list { $$ = CONS(jz_pleaf_new(jz, jz_parse_expr), reverse_list(jz, $1)); }

expr_list: assign_expr { $$ = CONS($1, NULL); }
  | expr_list COMMA assign_expr { $$ = CONS($3, $1); }
stmt_expr_list: stmt_assign_expr { $$ = CONS($1, NULL); }
  | stmt_expr_list COMMA assign_expr { $$ = CONS($3, $1); }

assign_expr: cond_expr
  | left_hand_expr assign_expr_op assign_expr { $$ = binop_node(jz, $2, $1, $3); }
stmt_assign_expr: stmt_cond_expr
  | stmt_left_hand_expr assign_expr_op assign_expr { $$ = binop_node(jz, $2, $1, $3); }

assign_expr_op: EQUALS { $$ = jz_op_assign; }
  | TIMES_EQ   { $$ = jz_op_times_eq; }
  | DIV_EQ     { $$ = jz_op_div_eq; }
  | MOD_EQ     { $$ = jz_op_mod_eq; }
  | PLUS_EQ    { $$ = jz_op_add_eq; }
  | MINUS_EQ   { $$ = jz_op_sub_eq; }
  | LSHIFT_EQ  { $$ = jz_op_lshift_eq; }
  | RSHIFT_EQ  { $$ = jz_op_rshift_eq; }
  | URSHIFT_EQ { $$ = jz_op_urshift_eq; }
  | BW_AND_EQ  { $$ = jz_op_bw_and_eq; }
  | XOR_EQ     { $$ = jz_op_xor_eq; }
  | BW_OR_EQ   { $$ = jz_op_bw_or_eq; }


cond_expr: or_expr
  | or_expr QUESTION assign_expr COLON assign_expr {
    $$ = jz_pnode_list(jz, 5, jz_pleaf_new(jz, jz_parse_triop),
                       jz_pleaf_new(jz, jz_op_cond), $1, $3, $5);
 }
stmt_cond_expr: stmt_or_expr
  | stmt_or_expr QUESTION assign_expr COLON assign_expr {
    $$ = jz_pnode_list(jz, 5, jz_pleaf_new(jz, jz_parse_triop),
                       jz_pleaf_new(jz, jz_op_cond), $1, $3, $5);
 }

or_expr: and_expr
  | or_expr OR and_expr { $$ = binop_node(jz, jz_op_or, $1, $3); }
stmt_or_expr: stmt_and_expr
  | stmt_or_expr OR and_expr { $$ = binop_node(jz, jz_op_or, $1, $3); }

and_expr: bw_or_expr
  | and_expr AND bw_or_expr { $$ = binop_node(jz, jz_op_and, $1, $3); }
stmt_and_expr: stmt_bw_or_expr
  | stmt_and_expr AND bw_or_expr { $$ = binop_node(jz, jz_op_and, $1, $3); }

bw_or_expr: xor_expr
  | bw_or_expr BW_OR xor_expr { $$ = binop_node(jz, jz_op_bw_or, $1, $3); }
stmt_bw_or_expr: stmt_xor_expr
  | stmt_bw_or_expr BW_OR xor_expr { $$ = binop_node(jz, jz_op_bw_or, $1, $3); }

xor_expr: bw_and_expr
  | xor_expr XOR bw_and_expr { $$ = binop_node(jz, jz_op_xor, $1, $3); }
stmt_xor_expr: stmt_bw_and_expr
  | stmt_xor_expr XOR bw_and_expr { $$ = binop_node(jz, jz_op_xor, $1, $3); }

bw_and_expr: eq_expr
  | bw_and_expr BW_AND eq_expr { $$ = binop_node(jz, jz_op_bw_and, $1, $3); }
stmt_bw_and_expr: stmt_eq_expr
  | stmt_bw_and_expr BW_AND eq_expr { $$ = binop_node(jz, jz_op_bw_and, $1, $3); }

eq_expr: rel_expr
  | eq_expr eq_expr_op rel_expr { $$ = binop_node(jz, $2, $1, $3); }
  | eq_expr neq_expr_op rel_expr {
    /* TODO: Make this transformation elsewhere */
    $$ = unop_node(jz, jz_op_not, binop_node(jz, $2, $1, $3));
 }
stmt_eq_expr: stmt_rel_expr
  | stmt_eq_expr eq_expr_op rel_expr { $$ = binop_node(jz, $2, $1, $3); }
  | stmt_eq_expr neq_expr_op rel_expr {
    $$ = unop_node(jz, jz_op_not, binop_node(jz, $2, $1, $3));
 }

eq_expr_op: EQ_EQ { $$ = jz_op_equals; }
  | STRICT_EQ { $$ = jz_op_strict_eq; }

neq_expr_op: NOT_EQ { $$ = jz_op_equals; }
  | NOT_STRICT_EQ { $$ = jz_op_strict_eq; }


rel_expr: shift_expr
  | rel_expr rel_expr_op shift_expr { $$ = binop_node(jz, $2, $1, $3); }
stmt_rel_expr: stmt_shift_expr
  | stmt_rel_expr rel_expr_op shift_expr { $$ = binop_node(jz, $2, $1, $3); }

rel_expr_op: LESS_THAN { $$ = jz_op_lt; }
  | GREATER_THAN { $$ = jz_op_gt; }
  | LT_EQ        { $$ = jz_op_lt_eq; }
  | GT_EQ        { $$ = jz_op_gt_eq; }


shift_expr: add_expr
  | shift_expr shift_expr_op add_expr { $$ = binop_node(jz, $2, $1, $3); }
stmt_shift_expr: stmt_add_expr
  | stmt_shift_expr shift_expr_op add_expr { $$ = binop_node(jz, $2, $1, $3); }

shift_expr_op: LSHIFT { $$ = jz_op_lshift; }
  | RSHIFT  { $$ = jz_op_rshift; }
  | URSHIFT { $$ = jz_op_urshift; }


add_expr: mult_expr
  | add_expr add_expr_op mult_expr { $$ = binop_node(jz, $2, $1, $3); }
stmt_add_expr: stmt_mult_expr
  | stmt_add_expr add_expr_op mult_expr { $$ = binop_node(jz, $2, $1, $3); }

add_expr_op: PLUS { $$ = jz_op_add; }
  | MINUS { $$ = jz_op_sub; }


mult_expr: unary_expr
  | mult_expr mult_expr_op unary_expr { $$ = binop_node(jz, $2, $1, $3); }
stmt_mult_expr: stmt_unary_expr
  | stmt_mult_expr mult_expr_op unary_expr { $$ = binop_node(jz, $2, $1, $3); }

mult_expr_op: TIMES { $$ = jz_op_times; }
  | DIV { $$ = jz_op_div; }
  | MOD { $$ = jz_op_mod; }


unary_expr: postfix_expr
  | unary_expr_op unary_expr { $$ = unop_node(jz, $1, $2); }
stmt_unary_expr: stmt_postfix_expr
  | unary_expr_op unary_expr { $$ = unop_node(jz, $1, $2); }

unary_expr_op: PLUS { $$ = jz_op_add; }
  | MINUS       { $$ = jz_op_sub; }
  | BW_NOT      { $$ = jz_op_bw_not; }
  | NOT         { $$ = jz_op_not; }
  | PLUS_PLUS   { $$ = jz_op_pre_inc; }
  | MINUS_MINUS { $$ = jz_op_pre_dec; }


postfix_expr: left_hand_expr
  | left_hand_expr postfix_expr_op { $$ = unop_node(jz, $2, $1); }
stmt_postfix_expr: stmt_left_hand_expr
  | stmt_left_hand_expr postfix_expr_op { $$ = unop_node(jz, $2, $1); }

postfix_expr_op: PLUS_PLUS { $$ = jz_op_post_inc; }
  | MINUS_MINUS { $$ = jz_op_post_dec; }


left_hand_expr: new_expr | call_expr
stmt_left_hand_expr: stmt_new_expr | stmt_call_expr

call_expr: call_or_member_expr arguments { $$ = CONS(jz_pleaf_new(jz, jz_parse_call), CONS($1, $2)); }
  | call_expr member_accessor { $$ = binop_node(jz, jz_op_index, $1, $2); }
stmt_call_expr: stmt_call_or_member_expr arguments { $$ = CONS(jz_pleaf_new(jz, jz_parse_call), CONS($1, $2)); }
  | stmt_call_expr member_accessor { $$ = binop_node(jz, jz_op_index, $1, $2); }

arguments: LPAREN RPAREN { $$ = NULL; }
  | LPAREN argument_list RPAREN { $$ = reverse_list(jz, $2); }

argument_list: assign_expr { $$ = CONS($1, NULL); }
  | argument_list COMMA assign_expr { $$ = CONS($3, $1); }

call_or_member_expr: member_expr | call_expr
stmt_call_or_member_expr: stmt_member_expr | stmt_call_expr

new_expr: member_expr
stmt_new_expr: stmt_member_expr

member_expr: primary_expr | function_expr
  | member_expr member_accessor { $$ = binop_node(jz, jz_op_index, $1, $2); }
stmt_member_expr: stmt_primary_expr
  | stmt_member_expr member_accessor { $$ = binop_node(jz, jz_op_index, $1, $2); }

member_accessor: LSQUARE expr RSQUARE { $$ = $2; }
  /* TODO: This is hideous and belongs in a compilation transformation. */
  | DOT identifier { $$ = jz_pnode_wrap(jz, jz_parse_literal, $2->cdr.node->car.str); }

function_expr: FUNCTION LPAREN RPAREN LCURLY opt_source_elements RCURLY {
  $$ = jz_pnode_wrap(jz, jz_parse_func, $5);
 }

opt_source_elements: source_elements
  | /* empty */ { $$ = NULL; }

primary_expr: stmt_primary_expr | object_literal
stmt_primary_expr: identifier | literal
  | THIS { $$ = CONS(jz_pleaf_new(jz, jz_parse_this), NULL); }
  | LPAREN expr RPAREN { $$ = $2; }

identifier: IDENTIFIER {
  $$ = jz_pnode_wrap(jz, jz_parse_identifier, jz_str_deep_dup(jz, $1));
 }

literal: literal_tval { $$ = jz_pnode_wrap(jz, jz_parse_literal, jz_tval_to_pleaf(jz, $1)); }

literal_tval: NUMBER { $$ = jz_wrap_num(jz, $1); }
  | STRING { $$ = jz_wrap_str(jz, $1); }
  | bool_val { $$ = jz_wrap_bool(jz, $1); }
  | NULL_VAL { $$ = JZ_NULL; }

bool_val: TRUE_VAL { $$ = jz_true; }
  | FALSE_VAL { $$ = jz_false; }

object_literal: LCURLY RCURLY {
  $$ = jz_pnode_wrap(jz, jz_parse_literal, jz_obj_new(jz));
 }

%%

jz_parse_node* jz_pnode_list(JZ_STATE, int argc, ...) {
  jz_parse_node* to_ret;
  jz_parse_node* end;
  va_list args;
  int i;

  if (argc == 0)
    return NULL;

  va_start(args, argc);

  to_ret = jz_pnode_new(jz);
  end = to_ret;
  to_ret->car.tag = va_arg(args, jz_tag*);

  for (i = 1; i < argc; i++) {
    jz_parse_node* next = jz_pnode_new(jz);

    end->cdr.node = next;
    end = next;
    end->car.tag = va_arg(args, jz_tag*);
  }

  va_end(args);

  end->cdr.tag = NULL;

  return to_ret;
}

jz_parse_node* jz_pnode_new(JZ_STATE) {
  jz_parse_node* to_ret = (jz_parse_node*)jz_gc_malloc(jz, jz_t_parse_node, sizeof(jz_parse_node));

  to_ret->car.node = to_ret->cdr.node = NULL;
  return to_ret;
}

jz_parse_leaf* jz_pleaf_new(JZ_STATE, unsigned char val) {
  jz_parse_leaf* to_ret = malloc(sizeof(jz_parse_leaf));
  to_ret->tag = JZ_TAG_WITH_TYPE(0, jz_t_enum);
  to_ret->val = val;
  return to_ret;
}

jz_parse_node* jz_pnode_cons(JZ_STATE, jz_tag* car, jz_tag* cdr) {
  jz_parse_node* to_ret = jz_pnode_new(jz);
  to_ret->car.tag = car;
  to_ret->cdr.tag = cdr;
  return to_ret;
}

jz_parse_ptr jz_tval_to_pleaf(JZ_STATE, jz_tvalue val) {
  jz_parse_ptr to_ret;

  if (JZ_TVAL_CAN_BE_GCED(val))
    to_ret.tag = (jz_tag*)(val.value.gc);
  else if (JZ_TVAL_IS_NULL(val))
    to_ret.tag = NULL;
  else {
    to_ret.val = malloc(sizeof(jz_tvalue));
    *to_ret.val = val;
  }

  return to_ret;
}

jz_tvalue jz_pleaf_to_tval(JZ_STATE, jz_parse_ptr val) {
  jz_tvalue to_ret;

  if (val.tag == NULL)
    return JZ_NULL;

  assert(JZ_TAG_TYPE(*val.tag) != jz_t_enum);
  assert(JZ_TAG_TYPE(*val.tag) != jz_t_parse_node);

  if (!JZ_TAG_CAN_BE_GCED(*val.tag))
    return *val.val;

  JZ_TVAL_SET_TYPE(to_ret, JZ_TAG_TYPE(*val.tag));
  to_ret.value.gc = val.gc;

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
  jz_lex_state* state = jz_lex_new(jz, code);
  int result = yyparse(jz, &root, state);

  free(state);

  /* yyparse returns 0 to indicate success. */
  if (result != 0)
    return NULL;

#if JZ_DEBUG_PARSE
  jz_print_parse_tree(jz, root);
#endif

  return root;
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

void jz_print_parse_tree(JZ_STATE, jz_parse_node* root) {
  print_parse_list(jz, root, jz_true);
  putchar('\n');
}

static void print_parse_list(JZ_STATE, jz_parse_node* node, jz_bool start) {
  if (start)
    putchar('(');

  print_parse_ptr(jz, node->car);

  if (node->cdr.tag == NULL)
    putchar(')');
  else if (JZ_TAG_TYPE(*node->cdr.tag) == jz_t_parse_node) {
    putchar(' ');
    print_parse_list(jz, node->cdr.node, jz_false);
  } else {
    printf(" . ");
    print_parse_ptr(jz, node->cdr);
    putchar(')');
  }
}

static void print_parse_ptr(JZ_STATE, jz_parse_ptr ptr) {
  if (ptr.tag == NULL) {
    printf("null");
    return;
  }

  switch (JZ_TAG_TYPE(*ptr.tag)) {
  case jz_t_enum:
    printf("[enum %d]", ptr.leaf->val);
    break;

  case jz_t_parse_node:
    print_parse_list(jz, ptr.node, jz_true);
    break;

  default: {
    char* str = jz_str_to_chars(jz, jz_to_str(jz, jz_pleaf_to_tval(jz, ptr)));
    printf("%s", str);
    free(str);
  }
  }
}

void jz_pnode_free(JZ_STATE, jz_parse_node* this) {
  free_parse_ptr(jz, this->car);
  free_parse_ptr(jz, this->cdr);
}

void free_parse_ptr(JZ_STATE, jz_parse_ptr ptr) {
  if (ptr.tag == NULL)
    return;
  else if (JZ_TAG_CAN_BE_GCED(*ptr.tag))
    return;
  else
    free(ptr.tag);
}

void yyerror(JZ_STATE, jz_parse_node** root, jz_lex_state* state, const char* msg)
{
  fprintf(stderr, "%s\n", msg);
}
