#ifndef JZ_FRAME_H
#define JZ_FRAME_H

#include "state.h"

jz_frame* jz_frame_new(JZ_STATE, const jz_bytecode* function);

#define JZ_FRAME_STACK(frame) \
  ((frame)->locals_then_stack + (frame)->bytecode->locals_length)
#define JZ_FRAME_LOCALS(frame) ((frame)->locals_then_stack)

#endif
