#ifndef JZ_VM_H
#define JZ_VM_H

#include "object.h"
#include "opcode.h"
#include "compile.h"

jz_tvalue jz_vm_run(jz_bytecode* bytecode);

#endif
