#include <stdlib.h>
#include <stdio.h>

#include "frame.h"
#include "state.h"
#include "function.h"
#include "object.h"

static void init_locals(JZ_STATE, jz_frame* frame);
static void copy_closure_locals(JZ_STATE, const jz_bytecode* function, jz_frame* frame);
static void copy_closure_vars(JZ_STATE, jz_closure_locals* vars, jz_frame* frame);

jz_frame* jz_frame_new_from_func(JZ_STATE, jz_obj* function) {
  jz_func_data* data = JZ_FUNC_DATA(function);
  jz_frame* frame = jz_frame_new(jz, data->code);

  frame->function = function;
  frame->closure_locals->scope = data->scope;
  copy_closure_vars(jz, data->scope, frame);

  return frame;
}

jz_frame* jz_frame_new(JZ_STATE, const jz_bytecode* function) {
  /* Use calloc to ensure the frame is initially zeroed. */
  jz_frame* frame;
  size_t extra_size = function->locals_length * sizeof(jz_val) +
    function->closure_vars_length * sizeof(jz_val*) - 1;

  frame = (jz_frame*)jz->stack;
  jz->stack += sizeof(jz_frame) + sizeof(jz_val) * extra_size;
  jz_check_overflow(jz, NULL);
  memset(frame, 0, sizeof(jz_frame) + extra_size);

  frame->function = NULL;
  frame->bytecode = function;
  frame->stack_top = NULL;
  frame->upper = jz->current_frame;
  jz->current_frame = frame;

  init_locals(jz, frame);

  frame->closure_locals = (jz_closure_locals*)
    jz_gc_dyn_malloc(jz, jz_t_closure_locals, sizeof(jz_closure_locals),
                     sizeof(jz_val), function->closure_locals_length);
  if (jz_gc_write_barrier_active(jz))
    jz_gc_mark_gray(jz, &frame->closure_locals->gc);

  frame->closure_locals->scope = NULL;
  frame->closure_locals->length = function->closure_locals_length;
  copy_closure_locals(jz, function, frame);

  return frame;
}

void init_locals(JZ_STATE, jz_frame* frame) {
  jz_val* locals = JZ_FRAME_LOCALS(frame);
  jz_val* top = JZ_FRAME_STACK(frame);

  for (; locals < top; locals++)
    *locals = JZ_UNDEFINED;
}

void copy_closure_locals(JZ_STATE, const jz_bytecode* function, jz_frame* frame) {
  jz_val* closure_locals = frame->closure_locals->vars;
  jz_val** closure_vars = JZ_FRAME_CLOSURE_VARS(frame) +
    function->closure_vars_length - function->closure_locals_length;
  jz_val** top = JZ_FRAME_CLOSURE_VARS(frame) + function->closure_vars_length;

  for (; closure_vars < top; closure_vars++, closure_locals++) {
    *closure_vars = closure_locals;
  }
}

void copy_closure_vars(JZ_STATE, jz_closure_locals* vars, jz_frame* frame) {
  jz_val* closure_locals = vars->vars;
  jz_val** closure_vars = JZ_FRAME_CLOSURE_VARS(frame);
  jz_val** top = JZ_FRAME_CLOSURE_VARS(frame) + vars->length;

  for (; closure_vars < top; closure_vars++, closure_locals++) {
    *closure_vars = closure_locals;
  }
}

void jz_frame_free(JZ_STATE, jz_frame* frame) {
  jz->current_frame = frame->upper;
  jz->stack = (jz_byte*)frame;
}
