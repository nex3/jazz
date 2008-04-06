#ifndef JZ_VM_H
#define JZ_VM_H

#include "jazz.h"
#include "value.h"
#include "compile.h"
#include "frame.h"

jz_val jz_vm_run(JZ_STATE, jz_bytecode* bytecode);
jz_val jz_vm_run_frame(JZ_STATE, jz_frame* frame);

#endif
