#include <stdlib.h>

#include "frame.h"

jz_frame* jz_frame_new(JZ_STATE, const jz_bytecode* function) {
  /* Use calloc to ensure the frame is initially zeroed. */
  jz_frame* frame;
  int extra_size = function->stack_length + function->locals_length - 1;

  frame = calloc(sizeof(jz_frame) + sizeof(jz_tvalue) * extra_size, 1);
  frame->bytecode = function;
  frame->stack_top = NULL;

  return frame;
}

void jz_frame_free(JZ_STATE, jz_frame* frame) {
  free(frame);
}
