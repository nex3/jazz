#ifndef JZ__CONS_H
#define JZ__CONS_H

/* This file contains various useful cons-related macros
   that don't belong in the global namespace.
   They don't necessarily obey normal Jazz macro conventions;
   for example, they aren't namespaced under "jz_",
   and they implicitly depend on a jz_state variable named "jz" existing. */

#include <assert.h>

#include "cons.h"

#define TYPE(val) JZ_TAG_TYPE(*(val).tag)
#define ASSERT_NODE(val) \
  assert((val).node == NULL || TYPE(val) == jz_t_cons)
#define NODE(val) (ASSERT_NODE(val), (val).node)
#define ENUM(value) (assert(TYPE(value) == jz_t_enum), (value).leaf->val)

#define CONS(car, cdr) jz_cons_new(jz, (jz_tag*)(car), (jz_tag*)(cdr))

#define CAR(n) ((n)->car)
#define CDR(n) ((n)->cdr)
#define CAAR(n) (CAR(CAR(n).node))
#define CDAR(n) (CDR(CAR(n).node))
#define CADR(n) (CAR(CDR(n).node))
#define CDDR(n) (CDR(CDR(n).node))
#define CAAAR(n) (CAR(CAAR(n).node))
#define CAADR(n) (CAR(CADR(n).node))
#define CADDR(n) (CAR(CDDR(n).node))
#define CDDAR(n) (CDR(CDAR(n).node))
#define CDDDR(n) (CDR(CDDR(n).node))
#define CADDDR(n) (CAR(CDDDR(n).node))

#endif
