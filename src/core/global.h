#ifndef JZ_CORE_GLOBAL_H
#define JZ_CORE_GLOBAL_H

#include "jazz.h"
#include "value.h"

#include <unicode/ustdio.h>

jz_val jz_load(JZ_STATE, UFILE* file);

void jz_init_global(JZ_STATE);

#endif
