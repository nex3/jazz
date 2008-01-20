#ifndef JZ_VM_H
#define JZ_VM_H

#include "state.h"
#include "type.h"
#include "opcode.h"
#include "compile.h"

jz_tvalue jz_vm_run(JZ_STATE, const jz_bytecode* bytecode);

#endif
