#ifndef JZ__CONS_H
#define JZ__CONS_H

/* This file contains various useful cons-related macros
   that don't belong in the global namespace.
   They don't necessarily obey normal Jazz macro conventions;
   for example, they aren't namespaced under "jz_",
   and they implicitly depend on a jz_state variable named "jz" existing. */

#include <assert.h>

#include "cons.h"

#define TYPE(val) JZ_VAL_TYPE(val)
#define ASSERT_GC_TYPE(val, t) (assert(JZ_IS_GC_TYPE(val, t)))
#define ASSERT_NODE(val) \
  assert((val) == NULL || JZ_IS_GC_TYPE(val, jz_t_cons))
#define NODE(val) ((jz_cons*)(ASSERT_NODE(val), (val)))
#define ENUM(value) \
  (ASSERT_GC_TYPE(value, jz_t_enum), ((jz_enum*)(value))->val)

#define VOID(ptr) (jz_wrap_void(jz, ptr))
#define UNVOID(node, type) \
  ((type)(assert(JZ_VAL_TAG(node) == jz_tt_void), jz_unwrap_void(jz, node)))
#define CONS(car, cdr) jz_cons_new(jz, car, cdr)

#define CAR(n) (((jz_cons*)(n))->car)
#define CDR(n) (((jz_cons*)(n))->cdr)
#define CAAR(n) (CAR(CAR(n)))
#define CDAR(n) (CDR(CAR(n)))
#define CADR(n) (CAR(CDR(n)))
#define CDDR(n) (CDR(CDR(n)))
#define CAAAR(n) (CAR(CAAR(n)))
#define CAADR(n) (CAR(CADR(n)))
#define CADDR(n) (CAR(CDDR(n)))
#define CDDAR(n) (CDR(CDAR(n)))
#define CDDDR(n) (CDR(CDDR(n)))
#define CADDDR(n) (CAR(CDDDR(n)))

#endif
