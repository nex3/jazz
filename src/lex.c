#include "lex.h"
#include "y.tab.h"
#include "keywords.gp.c"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#include <unicode/uregex.h>

/* Struct hash_result is defined in keywords.gperf. */
typedef struct hash_result hash_result;

static struct {
  jz_str code;
  jz_str code_prev;
  URegularExpression* identifier_re;
  URegularExpression* whitespace_re;
  URegularExpression* punctuation_re;
  URegularExpression* decimal_literal_re1;
  URegularExpression* decimal_literal_re2;
  URegularExpression* decimal_literal_re3;
} state;

static bool try_decimal_literal();
static int try_punctuation();
static int try_identifier();
static bool try_re(URegularExpression* re);
static jz_str get_match(URegularExpression* re, int number);

static URegularExpression* create_re(char* pattern);

#define CHECK_ICU_ERROR(error) {                                        \
    if (U_FAILURE(error)) {                                             \
      fprintf(stderr, "ICU Error: %s\n", u_errorName(error));           \
      assert(0);                                                        \
    }                                                                   \
  }

int yylex() {
  int res;
  int to_ret = 0;

  try_re(state.whitespace_re);

  if ((res = try_identifier())) to_ret = res;
  else if (try_decimal_literal()) to_ret = NUMBER;
  else if ((res = try_punctuation())) to_ret = res;

#if JZ_DEBUG_LEX
  printf("Token: %d\n", to_ret);
#endif

  return to_ret;
}

bool try_decimal_literal() {
  URegularExpression* matched;

  if (try_re(state.decimal_literal_re1))
    matched = state.decimal_literal_re1;
  else if (try_re(state.decimal_literal_re2))
    matched = state.decimal_literal_re2;
  else if (try_re(state.decimal_literal_re3))
    matched = state.decimal_literal_re3;
  else return false;

  {
    char *num, *dec, *exp;

    num = jz_str_to_chars(get_match(matched, 1));
    dec = jz_str_to_chars(get_match(matched, 2));
    exp = jz_str_to_chars(get_match(matched, 3));

    yylval.num = (*num) ? atof(num) : 0.0;

    /* dec[0] is always '.' */
    if (dec[1]) yylval.num += atof(dec);
    if (*exp)   yylval.num *= pow(10.0, atof(exp));

    free(num);
    free(dec);
    free(exp);
    return true;
  }
}

int try_punctuation() {
  jz_str jz_match;
  char* match;
  const hash_result* result;

  if (!try_re(state.punctuation_re)) return false;

  jz_match = get_match(state.punctuation_re, 0);
  match = jz_str_to_chars(jz_match);
  result = in_word_set(match, jz_match.length);
  free(match);
  if (result) return result->token;

  printf("Lexer bug: Unrecognized punctuation: \"%s\" at length %d\n", match, state.code.length);
  return false;
}

int try_identifier() {
  jz_str jz_match;
  char* match;
  const hash_result* result;

  if (!try_re(state.identifier_re)) return false;

  jz_match = get_match(state.identifier_re, 0);
  match = jz_str_to_chars(jz_match);
  result = in_word_set(match, jz_match.length);
  free(match);
  if (result) return result->token;

  return IDENTIFIER;
}

bool try_re(URegularExpression* re) {
  UErrorCode error = U_ZERO_ERROR;  

  uregex_setText(re, state.code.value, state.code.length, &error);
  CHECK_ICU_ERROR(error);

  {
    UBool found = uregex_find(re, 0, &error);
    CHECK_ICU_ERROR(error);
    if (!found) return false;
  }

  {
    int change = uregex_end(re, 0, &error);
    CHECK_ICU_ERROR(error);

    state.code_prev = state.code;
    state.code.value  += change;
    state.code.length -= change;
  }
  return true;
}

jz_str get_match(URegularExpression* re, int number) {
  UErrorCode error = U_ZERO_ERROR;
  int start;
  int end;

  start = uregex_start(re, number, &error);
  CHECK_ICU_ERROR(error);
  end = uregex_end(re, number, &error);
  CHECK_ICU_ERROR(error);

  if (start == -1) return jz_str_null();
  return jz_str_substr(state.code_prev, start, end - start);
}

#define IDENTIFIER_START_RE         "[\\p{Lu}\\p{Ll}\\p{Lt}\\p{Lm}\\p{Lo}\\p{Nl}\\$_]"
#define IDENTIFIER_PART_RE          "(?:" IDENTIFIER_START_RE "|[\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}])"
#define DECIMAL_INTEGER_LITERAL_RE  "(0|[1-9][0-9]*)"
#define EXPONENT_PART_RE            "(?:[eE]([\\+\\-]?[0-9]+))"

void jz_lex_set_code(jz_str code) {
  state.code = code;
  state.code_prev = code;
}

void jz_lex_init() {
  state.identifier_re       = create_re("\\A" IDENTIFIER_START_RE IDENTIFIER_PART_RE "*");
  state.whitespace_re       = create_re("\\A[\\p{Zs}\\t\\x0B\\f]");
  state.punctuation_re      = create_re("\\A(?:[\\{\\}\\(\\)\\[\\]\\.;,~\\?:]|"
                                       ">=|<=|!=?=?|\\+=?|-=?|\\*=?|%=?|<<=|>>=|&=?|\\|=?|\\^=?|\\/=?|"
                                       "<<?|>{1,3}|={1,3}|\\+\\+|--|&&|\\|\\|)");
  state.decimal_literal_re1 = create_re("\\A" DECIMAL_INTEGER_LITERAL_RE "(\\.[0-9]*)" EXPONENT_PART_RE "?");
  state.decimal_literal_re2 = create_re("\\A()(\\.[0-9]+)" EXPONENT_PART_RE "?");
  state.decimal_literal_re3 = create_re("\\A" DECIMAL_INTEGER_LITERAL_RE "()" EXPONENT_PART_RE "?");
}

URegularExpression* create_re(char* pattern) {
  UErrorCode error = U_ZERO_ERROR;
  URegularExpression* re = uregex_openC(pattern, 0, NULL, &error);
  CHECK_ICU_ERROR(error);
  return re;
}
