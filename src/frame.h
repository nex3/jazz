#ifndef JZ_FRAME_H
#define JZ_FRAME_H

#include "jazz.h"
#include "compile.h"
#include "value.h"

typedef struct {
  const jz_bytecode* bytecode;
  jz_tvalue locals_then_stack[1];
} jz_frame;

jz_frame* jz_frame_new(JZ_STATE, const jz_bytecode* function);
void jz_frame_free(JZ_STATE, jz_frame* frame);

#define JZ_FRAME_STACK(frame) \
  ((frame)->locals_then_stack + (frame)->bytecode->locals_length)
#define JZ_FRAME_LOCALS(frame) ((frame)->locals_then_stack)

#endif
