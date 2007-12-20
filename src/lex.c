#include "lex.h"
#include "y.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unicode/uregex.h>

static struct {
  jz_str code;
  jz_str code_prev;
  URegularExpression* identifier_re;
  URegularExpression* whitespace_re;
  URegularExpression* decimal_literal_re;
} state;

static int try_decimal_literal();
static int try_re(URegularExpression* re, int type);
static jz_str get_match(URegularExpression* re, int number);

static URegularExpression* create_re(char* pattern);

static void check_error(UErrorCode error) {
  if (U_FAILURE(error)) fprintf(stderr, "ICU Error: %s\n", u_errorName(error));
}

int yylex() {
  try_re(state.whitespace_re, true);

  int res;
  if ((res = try_re(state.identifier_re, IDENTIFIER))) return res;
  if ((res = try_decimal_literal())) return res;
  return 0;
}

int try_decimal_literal() {
  if (!try_re(state.decimal_literal_re, true)) return false;
  char* num = jz_str_to_chars(get_match(state.decimal_literal_re, 0));
  yylval.num = (float)(atoi(num));
  free(num);
  return NUMBER;
}

int try_re(URegularExpression* re, int type) {
  UErrorCode error = U_ZERO_ERROR;  

  uregex_setText(re, state.code.value, state.code.length, &error);

  UBool found = uregex_find(re, 0, &error);
  check_error(error);
  if (!found) return false;

  int change = uregex_end(re, 0, &error);
  check_error(error);

  state.code_prev = state.code;
  state.code.value  += change;
  state.code.length -= change;
  return type;
}

jz_str get_match(URegularExpression* re, int number) {
  UErrorCode error = U_ZERO_ERROR;
  int start = uregex_start(re, number, &error);
  check_error(error);
  int end   = uregex_end(re, number, &error);
  check_error(error);

  return jz_str_substr(state.code_prev, start, end);
}

#define IDENTIFIER_START_RE         "[\\p{Lu}\\p{Ll}\\p{Lt}\\p{Lm}\\p{Lo}\\p{Nl}\\$_]"
#define IDENTIFIER_PART_RE          "(?:" IDENTIFIER_START_RE "|[\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}])"
#define DECIMAL_INTEGER_LITERAL_RE  "(?:0|[1-9][0-9]*)"

void jz_lex_set_code(jz_str code) {
  state.code = code;
  state.code_prev = code;
}

void jz_lex_init() {
  state.identifier_re      = create_re("\\A" IDENTIFIER_START_RE IDENTIFIER_PART_RE "+");
  state.whitespace_re      = create_re("\\A[\\p{Zs}\t\x0B\f]");
  state.decimal_literal_re = create_re(DECIMAL_INTEGER_LITERAL_RE);
}

URegularExpression* create_re(char* pattern) {
  UErrorCode error = U_ZERO_ERROR;
  URegularExpression* re = uregex_openC(pattern, 0, NULL, &error);
  check_error(error);
  return re;
}
