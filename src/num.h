#ifndef JZ_NUM_H
#define JZ_NUM_H

#include "jazz.h"
#include "value.h"

struct jz_num {
  jz_gc_header gc;
  double num;
};

#endif
