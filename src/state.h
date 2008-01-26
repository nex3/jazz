#ifndef JZ_STATE_H
#define JZ_STATE_H

#include <unicode/uregex.h>
#include "jazz.h"
#include "value.h"
#include "frame.h"
#include "gc.h"

struct jz_state {
  jz_frame* current_frame;
  jz_tvalue undefined_val;
  jz_tvalue null_val;
  jz_tvalue true_val;
  jz_tvalue false_val;
  struct {
    unsigned char state;
    unsigned char speed;
    unsigned char pause;
    size_t allocated;
    size_t threshold;
    bool black_bit;
    jz_gc_header* all_objs;
    jz_gc_node* gray_stack;
    jz_gc_header* prev_sweep_obj;
    jz_gc_header* next_sweep_obj;
  } gc;
  struct {
    URegularExpression* identifier_re;
    URegularExpression* whitespace_re;
    URegularExpression* line_terminator_re;
    URegularExpression* punctuation_re;
    URegularExpression* hex_literal_re;
    URegularExpression* decimal_literal_re1;
    URegularExpression* decimal_literal_re2;
    URegularExpression* decimal_literal_re3;
    URegularExpression* string_literal_re;
  } lex;
};

#define JZ_UNDEFINED (jz->undefined_val)
#define JZ_NULL      (jz->null_val)
#define JZ_TRUE      (jz->true_val)
#define JZ_FALSE     (jz->false_val)

jz_state* jz_init();

void jz_free_state(JZ_STATE);

#endif
