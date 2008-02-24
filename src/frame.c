#include <stdlib.h>

#include "frame.h"
#include "state.h"
#include "function.h"
#include "object.h"

static void copy_closure_locals(JZ_STATE, const jz_bytecode* function, jz_frame* frame);
static void copy_closure_vars(JZ_STATE, jz_closure_locals* vars, jz_frame* frame);

jz_frame* jz_frame_new_from_func(JZ_STATE, jz_obj* function) {
  jz_frame* frame = jz_frame_new(jz, JZ_FUNC_DATA(function)->code);

  frame->function = function;
  copy_closure_vars(jz, JZ_FUNC_DATA(function)->scope, frame);
  return frame;
}

jz_frame* jz_frame_new(JZ_STATE, const jz_bytecode* function) {
  /* Use calloc to ensure the frame is initially zeroed. */
  jz_frame* frame;
  size_t extra_size = function->locals_length * sizeof(jz_tvalue) +
    function->closure_vars_length * sizeof(jz_tvalue*) - 1;

  frame = (jz_frame*)jz->stack;
  memset(frame, 0, sizeof(jz_frame) + extra_size);

  jz->stack += sizeof(jz_frame) + sizeof(jz_tvalue) * extra_size;
  frame->function = NULL;
  frame->bytecode = function;
  frame->stack_top = NULL;
  frame->upper = jz->current_frame;
  jz->current_frame = frame;

  frame->closure_locals = (jz_closure_locals*)
    jz_gc_dyn_malloc(jz, jz_t_closure_locals, sizeof(jz_closure_locals),
                     sizeof(jz_tvalue), function->closure_locals_length);
  frame->closure_locals->scope = NULL; /* TODO: Actually handle this */
  frame->closure_locals->length = function->closure_locals_length;
  copy_closure_locals(jz, function, frame);

  return frame;
}

void copy_closure_locals(JZ_STATE, const jz_bytecode* function, jz_frame* frame) {
  jz_tvalue* closure_locals = frame->closure_locals->vars;
  jz_tvalue** closure_vars = JZ_FRAME_CLOSURE_VARS(frame) +
    function->closure_vars_length - function->closure_locals_length;
  jz_tvalue** top = JZ_FRAME_CLOSURE_VARS(frame) + function->closure_vars_length;

  for (; closure_vars < top; closure_vars++, closure_locals++) {
    *closure_vars = closure_locals;
  }
}

void copy_closure_vars(JZ_STATE, jz_closure_locals* vars, jz_frame* frame) {
  jz_tvalue* closure_locals = vars->vars;
  jz_tvalue** closure_vars = JZ_FRAME_CLOSURE_VARS(frame);
  jz_tvalue** top = JZ_FRAME_CLOSURE_VARS(frame) + vars->length;

  for (; closure_vars < top; closure_vars++, closure_locals++) {
    *closure_vars = closure_locals;
  }
}

void jz_frame_free(JZ_STATE, jz_frame* frame) {
  jz->stack = (jz_byte*)frame;
}
