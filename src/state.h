#ifndef JZ_STATE_H
#define JZ_STATE_H

#include <unicode/uregex.h>
#include "jazz.h"
#include "value.h"
#include "frame.h"
#include "gc.h"

struct jz_state {
  jz_byte* stack;
  jz_frame* current_frame;
  jz_obj* prototypes;
  jz_obj* global_obj;
  struct {
    jz_byte state;
    jz_byte speed;
    jz_byte pause;
    size_t allocated;
    size_t threshold;
    jz_bool black_bit;
    jz_gc_header* all_objs;
    jz_gc_node* gray_stack;
    jz_gc_header* prev_sweep_obj;
    jz_gc_header* next_sweep_obj;
  } gc;
  struct {
    URegularExpression* identifier_re;
    URegularExpression* whitespace_re;
    URegularExpression* line_terminator_re;
    URegularExpression* one_line_comment_re;
    URegularExpression* multiline_comment_re;
    URegularExpression* punctuation_re;
    URegularExpression* hex_literal_re;
    URegularExpression* decimal_literal_re1;
    URegularExpression* decimal_literal_re2;
    URegularExpression* decimal_literal_re3;
    URegularExpression* string_literal_re;
  } lex;
};

jz_state* jz_init();

void jz_free_state(JZ_STATE);

#endif
