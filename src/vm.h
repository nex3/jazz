#ifndef JZ_VM_H
#define JZ_VM_H

#include "jazz.h"
#include "value.h"
#include "compile.h"

jz_tvalue jz_vm_run(JZ_STATE, const jz_bytecode* bytecode);

#endif
