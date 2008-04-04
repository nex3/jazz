#ifndef JZ_FRAME_H
#define JZ_FRAME_H

#include "jazz.h"
#include "compile.h"
#include "value.h"

typedef struct jz_closure_locals jz_closure_locals;
struct jz_closure_locals {
  jz_gc_header gc;
  jz_closure_locals* scope;
  size_t length;
  jz_val vars[1];
};

struct jz_frame {
  jz_bool mark;
  jz_obj* function;
  const jz_bytecode* bytecode;
  jz_frame* upper;
  jz_closure_locals* closure_locals;
  /* This points to the stack variable in jz_vm_run.
     It allows the garbage collector to determine
     where the stack ends. */
  jz_val** stack_top;
  /* This contains various data used by the frame.
     From top (index 0) to bottom:
     * Pointers to closure variables (jz_val*)
     * Local variables (jz_val)
     * The stack (jz_val) */
  char data[1];
};

/* Note: frames are allocated and freed on the Jazz stack. */
jz_frame* jz_frame_new_from_func(JZ_STATE, jz_obj* function);
jz_frame* jz_frame_new(JZ_STATE, const jz_bytecode* function);
void jz_frame_free(JZ_STATE, jz_frame* frame);

#define JZ_FRAME_STACK(frame)                                           \
  ((jz_val*)((frame)->data +                                            \
             (frame)->bytecode->closure_vars_length * sizeof(jz_val*) + \
             (frame)->bytecode->locals_length * sizeof(jz_val)))
#define JZ_FRAME_LOCALS(frame)                                          \
  ((jz_val*)((frame)->data +                                            \
                 (frame)->bytecode->closure_vars_length * sizeof(jz_val*)))
#define JZ_FRAME_CLOSURE_VARS(frame) ((jz_val**)((frame)->data))

#endif
