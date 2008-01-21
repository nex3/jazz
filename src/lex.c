/* Lexing mostly works by applying a series of regular expressions
   to the source text,
   and returning the token corresponding with the first one that succeeds.
   However, some matches must be interpreted in various ways,
   so it gets slightly more complicated.

   We can't use a lex derivative for this because
   none of them appear to support UTF-16 well. */

#include "lex.h"
#include "y.tab.h"
#include "keywords.gp.c"
#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

/* Struct hash_result is defined in keywords.gperf. */
typedef struct hash_result hash_result;

#define STATE JZ_STATE, jz_lex_state* state

static bool try_filler(STATE);
static bool try_hex_literal(STATE, YYSTYPE* lex_val);
static bool try_decimal_literal(STATE, YYSTYPE* lex_val);
static bool try_string_literal(STATE, YYSTYPE* lex_val);
typedef const UChar* const_uchars;
static UChar hex_escape(STATE, int chars, const jz_str* match,
                        const_uchars* match_ptr_ptr);
static int try_punctuation(STATE, YYSTYPE* lex_val);
static int try_identifier(STATE, YYSTYPE* lex_val);


/* Attempts to apply 're' to the current position of the code string.
   Returns true if successful, false otherwise. */
static bool try_re(STATE, URegularExpression* re);

/* Returns a jz_str* of the text for the given match of the given regexp.
   Match 0 is the entire matched string.
   Returns NULL if there is no match.

   The return value should be freed when no longer in use.
   The string is not a deep copy,
   so the value should not be freed. */
static jz_str* get_match(STATE, URegularExpression* re, int number);

#define CHECK_ICU_ERROR(error) {                                        \
    if (U_FAILURE(error)) {                                             \
      fprintf(stderr, "ICU Error at %d: %s\n", __LINE__, u_errorName(error)); \
      exit(1);                                                          \
    }                                                                   \
  }

double jz_parse_number(JZ_STATE, const jz_str* num) {
  jz_lex_state* state = jz_lex_init(jz, num);
  YYSTYPE lval;
  double res;

  lval.num = JZ_NAN;
  if (try_hex_literal(jz, state, &lval));
  else if (try_decimal_literal(jz, state, &lval));
  res = lval.num;

  /* Make sure that we consumed the entire string. */
  if (state->code->length != 0)
    res = JZ_NAN;

  free(state);

  return res;
}

int yylex(YYSTYPE* lex_val, STATE) {
  int res;
  int to_ret = 0;

  assert(state->code != NULL);

  while (try_filler(jz, state));

  if ((res = try_identifier(jz, state, lex_val))) to_ret = res;
  else if (try_hex_literal(jz, state, lex_val)) to_ret = NUMBER;
  else if (try_decimal_literal(jz, state, lex_val)) to_ret = NUMBER;
  else if (try_string_literal(jz, state, lex_val)) to_ret = STRING;
  else if ((res = try_punctuation(jz, state, lex_val))) to_ret = res;

#if JZ_DEBUG_LEX
  printf("Token: %d\n", to_ret);
#endif

  return to_ret;
}

bool try_filler(STATE) {
  if (try_re(jz, state, jz->lex.whitespace_re)) return true;
  if (try_re(jz, state, jz->lex.line_terminator_re)) return true;
  return false;
}

bool try_decimal_literal(STATE, YYSTYPE* lex_val) {
  URegularExpression* matched;

  if (try_re(jz, state, jz->lex.decimal_literal_re1))
    matched = jz->lex.decimal_literal_re1;
  else if (try_re(jz, state, jz->lex.decimal_literal_re2))
    matched = jz->lex.decimal_literal_re2;
  else if (try_re(jz, state, jz->lex.decimal_literal_re3))
    matched = jz->lex.decimal_literal_re3;
  else return false;

  {
    jz_str* match;
    char *num, *dec, *exp;

    match = get_match(jz, state, matched, 1);
    num = jz_str_to_chars(jz, match);
    free(match);

    match = get_match(jz, state, matched, 2);
    dec = jz_str_to_chars(jz, match);
    free(match);

    match = get_match(jz, state, matched, 3);
    exp = jz_str_to_chars(jz, match);
    free(match);

    lex_val->num = (*num) ? atof(num) : 0.0;

    /* dec[0] is always '.' if it's not '\0' */
    if (dec[0] && dec[1]) lex_val->num += atof(dec);
    if (*exp) lex_val->num *= pow(10.0, atof(exp));

    free(num);
    free(dec);
    free(exp);
    return true;
  }
}

bool try_hex_literal(STATE, YYSTYPE* lex_val) {
  if (!try_re(jz, state, jz->lex.hex_literal_re)) return false;

  {
    jz_str* match = get_match(jz, state, jz->lex.hex_literal_re, 0);
    char *num = jz_str_to_chars(jz, match);
    unsigned int hex;
    free(match);

    sscanf(num + 2, "%x", &hex);
    lex_val->num = (double)hex;
    free(num);
    return true;
  }
}

bool try_string_literal(STATE, YYSTYPE* lex_val) {
  if (!try_re(jz, state, jz->lex.string_literal_re)) return false;

  {
    jz_str* match = get_match(jz, state, jz->lex.string_literal_re, 2);
    UChar* res = calloc(sizeof(UChar), match->length);
    UChar* res_bottom = res;
    const UChar* match_ptr_bottom = JZ_STR_PTR(match);
    const UChar* match_ptr = match_ptr_bottom;

    for (; match_ptr - match_ptr_bottom < match->length; match_ptr++) {
      UChar val = *match_ptr;

      if (val != '\\') {
        *res++ = val;
        continue;
      }

      val = *++match_ptr;
      switch (val) {
      case 'b':
        *res++ = '\b';
        break;
      case 't':
        *res++ = '\t';
        break;
      case 'n':
        *res++ = '\n';
        break;
      case 'v':
        *res++ = '\v';
        break;
      case 'f':
        *res++ = '\f';
        break;
      case 'r':
        *res++ = '\r';
        break;
      case '0':
        *res++ = '\0';
        break;
      case 'x':
        *res++ = hex_escape(jz, state, 2, match, &match_ptr);
        break;
      case 'u':
        *res++ = hex_escape(jz, state, 4, match, &match_ptr);
        break;
      default:
        *res++ = val;
        break;
      }
    }

    lex_val->str = jz_str_external(jz, res - res_bottom, res_bottom);
    free(match);
    return true;
  }
}

#define VALID_HEX_CHAR(c)                       \
  (((c) >= '0' && (c) <= '9') ||                \
   ((c) >= 'a' && (c) <= 'f') ||                \
   ((c) >= 'A' && (c) <= 'F'))

#define PRINT_HEX_ERROR {                                               \
    fprintf(stderr, "Invalid hex escape: \"%s\"\n", jz_str_to_chars(jz, match)); \
    exit(1);                                                            \
  }

UChar hex_escape(STATE, int chars, const jz_str* match, const_uchars* match_ptr_ptr) {
  const UChar* match_ptr = *match_ptr_ptr;
  match_ptr++;

  if (match->length - (match_ptr - JZ_STR_PTR(match)) < chars) PRINT_HEX_ERROR;

  {
    char* num = calloc(sizeof(char), chars + 1);
    unsigned int hex;
    int i;

    for (i = 0; i < chars; i++) {
      num[i] = *match_ptr++;

      if (!VALID_HEX_CHAR(num[i])) PRINT_HEX_ERROR
    }
    match_ptr--;

    num[chars] = '\0';

    *match_ptr_ptr = match_ptr;
    sscanf(num, "%x", &hex);
    free(num);

    return hex;
  }
}

int try_punctuation(STATE, YYSTYPE* lex_val) {
  jz_str* jz_match;
  char* match;
  const hash_result* result;

  if (!try_re(jz, state, jz->lex.punctuation_re)) return false;

  jz_match = get_match(jz, state, jz->lex.punctuation_re, 0);
  match = jz_str_to_chars(jz, jz_match);
  result = in_word_set(match, jz_match->length);

  free(jz_match);
  free(match);

  if (result) return result->token;

  printf("Lexer bug: Unrecognized punctuation: \"%s\" at length %d\n", match, state->code->length);
  return false;
}

int try_identifier(STATE, YYSTYPE* lex_val) {
  jz_str* jz_match;
  char* match;
  const hash_result* result;

  if (!try_re(jz, state, jz->lex.identifier_re)) return false;

  jz_match = get_match(jz, state, jz->lex.identifier_re, 0);
  match = jz_str_to_chars(jz, jz_match);
  result = in_word_set(match, jz_match->length);
  free(match);

  if (result) {
    free(jz_match);
    return result->token;
  }

  lex_val->str = jz_match;
  return IDENTIFIER;
}

bool try_re(STATE, URegularExpression* re) {
  UErrorCode error = U_ZERO_ERROR;

  if (state->code->length == 0) return false;

  uregex_setText(re, JZ_STR_PTR(state->code), state->code->length, &error);
  CHECK_ICU_ERROR(error);

  {
    UBool found = uregex_find(re, 0, &error);
    CHECK_ICU_ERROR(error);
    if (!found) return false;
  }

  {
    int change = uregex_end(re, 0, &error);
    CHECK_ICU_ERROR(error);

    free(state->code_prev);
    state->code_prev = state->code;
    state->code = jz_str_substr(jz, state->code, change);
  }
  return true;
}

jz_str* get_match(STATE, URegularExpression* re, int number) {
  UErrorCode error = U_ZERO_ERROR;
  int start;
  int end;

  start = uregex_start(re, number, &error);
  CHECK_ICU_ERROR(error);
  end = uregex_end(re, number, &error);
  CHECK_ICU_ERROR(error);

  if (start == -1) return NULL;
  return jz_str_substr2(jz, state->code_prev, start, end);
}

jz_lex_state* jz_lex_init(JZ_STATE, const jz_str* code) {
  jz_lex_state* state = malloc(sizeof(jz_lex_state));

  assert(code != NULL);

  state->code = jz_str_dup(jz, code);
  state->code_prev = jz_str_dup(jz, code);

  return state;
}
