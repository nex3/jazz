#ifndef JZ_TRAVERSE_H
#define JZ_TRAVERSE_H

#include "jazz.h"
#include "cons.h"

/* Returning true means "continue traversing sub-nodes",
   false means "go on to next sibling." */
typedef jz_bool jz_traverse_fn(JZ_STATE, jz_cons* node, void* data);

#define jz_traverse_one(jz, tree, fn, data, enum_val) \
  jz_traverse_several(jz, tree, fn, data, 1, enum_val)

/* Do a preorder traversal of tree,
   calling fn on nodes whose car is an enum
   containing a val in the varargs list. */
void jz_traverse_several(JZ_STATE, jz_cons* tree, jz_traverse_fn* fn, void* data,
                         int argc, ...);

#endif
