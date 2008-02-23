#include <stdlib.h>

#include "frame.h"
#include "state.h"

jz_frame* jz_frame_new(JZ_STATE, const jz_bytecode* function) {
  /* Use calloc to ensure the frame is initially zeroed. */
  jz_frame* frame;
  int extra_size = function->locals_length - 1;

  frame = (jz_frame*)jz->stack;
  memset(frame, 0, sizeof(jz_frame) + sizeof(jz_tvalue) * extra_size);

  jz->stack += sizeof(jz_frame) + sizeof(jz_tvalue) * extra_size;
  frame->bytecode = function;
  frame->stack_top = NULL;
  frame->upper = jz->current_frame;
  jz->current_frame = frame;

  return frame;
}

void jz_frame_free(JZ_STATE, jz_frame* frame) {
  jz->stack = (jz_byte*)frame;
}
