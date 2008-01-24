#include <stdlib.h>
#include <stdio.h>

#include "state.h"

static void init_gc(JZ_STATE);
static void init_lex(JZ_STATE);
static URegularExpression* create_re(const char* pattern);

static void free_lex(JZ_STATE);

jz_state* jz_init() {
  jz_state* state = malloc(sizeof(jz_state));

  /* Assign value constants. */
  JZ_TVAL_SET_TYPE(state->undefined_val, jz_undef);
  JZ_TVAL_SET_TYPE(state->true_val, jz_bool);
  state->true_val.value.b = true;
  JZ_TVAL_SET_TYPE(state->false_val, jz_bool);
  state->true_val.value.b = false;

  state->current_frame = NULL;

  init_gc(state);
  init_lex(state);

  return state;
}

void init_gc(JZ_STATE) {
  jz->gc.state = jz_gcs_waiting;
  jz->gc.speed = JZ_GC_DEFAULT_SPEED;
  jz->gc.black_bit = false;
  jz->gc.all_objs = NULL;
  jz->gc.gray_stack = NULL;
  jz->gc.prev_sweep_obj = NULL;
  jz->gc.next_sweep_obj = NULL;
}

/* All regular expressions should begin with \A so they only match
   at the beginning of the string.

   TODO: Move this into lex.c. */

#define CHECK_ICU_ERROR(error) {                                        \
    if (U_FAILURE(error)) {                                             \
      fprintf(stderr, "ICU Error at %d: %s\n", __LINE__, u_errorName(error)); \
      exit(1);                                                          \
    }                                                                   \
  }

#define IDENTIFIER_START_RE         "[\\p{Lu}\\p{Ll}\\p{Lt}\\p{Lm}\\p{Lo}\\p{Nl}\\$_]"
#define IDENTIFIER_PART_RE          "(?:" IDENTIFIER_START_RE "|[\\p{Mn}\\p{Mc}\\p{Nd}\\p{Pc}])"
#define DECIMAL_INTEGER_LITERAL_RE  "(0|[1-9][0-9]*)"
#define EXPONENT_PART_RE            "(?:[eE]([\\+\\-]?[0-9]+))"
#define LINE_TERMINATOR_CHARS       "\\n\\r\\u2028\\u2029"

void init_lex(JZ_STATE) {
  jz->lex.identifier_re       = create_re("\\A" IDENTIFIER_START_RE IDENTIFIER_PART_RE "*");
  jz->lex.whitespace_re       = create_re("\\A[\\p{Zs}\\t\\x0B\\f]");
  jz->lex.line_terminator_re  = create_re("\\A[" LINE_TERMINATOR_CHARS "]");
  jz->lex.punctuation_re      = create_re("\\A(?:[\\{\\}\\(\\)\\[\\]\\.;,~\\?:]|"
                                             ">>>=?|={1,3}|\\+\\+|--|&&|\\|\\||"
                                             "<<=?|>>=?|!=?=?|\\+=?|-=?|\\*=?|%=?|"
                                             ">=?|<=?|&=?|\\|=?|\\^=?|\\/=?)");
  jz->lex.hex_literal_re      = create_re("\\A0[xX][0-9a-fA-F]+");
  jz->lex.decimal_literal_re1 = create_re("\\A" DECIMAL_INTEGER_LITERAL_RE "(\\.[0-9]*)" EXPONENT_PART_RE "?");
  jz->lex.decimal_literal_re2 = create_re("\\A()(\\.[0-9]+)" EXPONENT_PART_RE "?");
  jz->lex.decimal_literal_re3 = create_re("\\A" DECIMAL_INTEGER_LITERAL_RE "()" EXPONENT_PART_RE "?");
  jz->lex.string_literal_re   = create_re("\\A('|\")((?:[^\\1\\\\" LINE_TERMINATOR_CHARS "]|"
                                             "\\\\(?:[^1-9" LINE_TERMINATOR_CHARS "]))*?)\\1");
}

URegularExpression* create_re(const char* pattern) {
  UErrorCode error = U_ZERO_ERROR;
  URegularExpression* re = uregex_openC(pattern, 0, NULL, &error);
  CHECK_ICU_ERROR(error);
  return re;
}

void jz_free_state(JZ_STATE) {
  free_lex(jz);
  free(jz);
}

void free_lex(JZ_STATE) {
  uregex_close(jz->lex.identifier_re);
  uregex_close(jz->lex.whitespace_re);
  uregex_close(jz->lex.line_terminator_re);
  uregex_close(jz->lex.punctuation_re);
  uregex_close(jz->lex.hex_literal_re);
  uregex_close(jz->lex.decimal_literal_re1);
  uregex_close(jz->lex.decimal_literal_re2);
  uregex_close(jz->lex.decimal_literal_re3);
  uregex_close(jz->lex.string_literal_re);
}
