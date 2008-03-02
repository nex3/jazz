#ifndef JZ_CONS_H
#define JZ_CONS_H

#include "jazz.h"
#include "gc.h"

typedef struct jz_cons jz_cons;

/* A tagged wrapper for enums.
   Pretty much just used for parse types at the moment. */
typedef struct {
  jz_gc_header gc;
  jz_byte val;
} jz_enum;

/* A cons cell. */
struct jz_cons {
  jz_gc_header gc;
  jz_val car;
  jz_val cdr;
};


/* Returns a new parse node with NULL car and cdr. */
jz_cons* jz_cons_empty(JZ_STATE);

jz_enum* jz_enum_new(JZ_STATE, jz_byte val);

jz_cons* jz_cons_new(JZ_STATE, jz_val car, jz_val cdr);

jz_cons* jz_list(JZ_STATE, int argc, ...);

/* Concatenates two lists.
   list1 is modified so that its last element
   points to the first element of list2.
   No new nodes are allocated.

   list1 is returned as a convenience, unless it's NULL,
   in which case list2 is returned. */
jz_cons* jz_list_concat(JZ_STATE, jz_cons* list1, jz_cons* list2);

/* Reverses an s-expression list and returns the new list.
   The list is destructively modified. */
jz_cons* jz_list_reverse(JZ_STATE, jz_cons* head);

void jz_cons_print(JZ_STATE, jz_cons* root);

#define jz_enum_wrap(jz, type, car) jz_list(jz, 2, jz_enum_new(jz, type), car)

#endif
