#include "lex.h"
#include "y.tab.h"
#include <unicode/uregex.h>

struct {
  jz_str code;
  URegularExpression* identifier_re;
  URegularExpression* whitespace_re;
} state;

#include <stdio.h>

static int try_re(URegularExpression* re, int type);
static URegularExpression* create_re(char* pattern);

static void check_error(UErrorCode error) {
  if (U_FAILURE(error)) fprintf(stderr, "ICU Error: %s\n", u_errorName(error));
}

int yylex() {
  try_re(state.whitespace_re, -1);

  int res;
  if ((res = try_re(state.identifier_re, IDENTIFIER))) return res;
  return 0;
}

int try_re(URegularExpression* re, int type) {
  UErrorCode error = U_ZERO_ERROR;  

  uregex_setText(re, state.code.value, state.code.length, &error);

  UBool found = uregex_find(re, 0, &error);
  check_error(error);
  if (!found) return false;

  int change = uregex_end(re, 0, &error);
  check_error(error);

  state.code.value  += change;
  state.code.length -= change;
  return type;
}

#define IDENTIFIER_START_RE "[\\p{Lu}\\p{Ll}\\p{Lt}\\p{Lm}\\p{Lo}\\p{Nl}\\$_]"
#define IDENTIFIER_PART_RE  "(" IDENTIFIER_START_RE "|[\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}])"

void jz_lex_set_code(jz_str code) {
  state.code = code;
}

void jz_lex_init() {
  state.identifier_re = create_re("\\A" IDENTIFIER_START_RE IDENTIFIER_PART_RE "+");
  state.whitespace_re = create_re("\\A[\\p{Zs}\t\x0B\f]");
}

URegularExpression* create_re(char* pattern) {
  UErrorCode error = U_ZERO_ERROR;
  URegularExpression* re = uregex_openC(pattern, 0, NULL, &error);
  check_error(error);
  return re;
}
