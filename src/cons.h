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

/* Potential values for the car and cdr of a jz_cons.

   Note that all of these should be pointers to values
   where the first element is a jz_tag.

   Also, in a cons the Jazz null value
   isn't represented by a pointer to a null tvalue,
   but rather an actual NULL pointer.
   This avoids ambiguity between null tvalues and objects. */
typedef union {
  jz_tag* tag;
  jz_gc_header* gc;
  jz_cons* node;
  jz_str* str;
  jz_tvalue* val;
  jz_enum* leaf;
} jz_cons_ptr;

/* A cons cell. */
struct jz_cons {
  jz_gc_header gc;
  jz_cons_ptr car;
  jz_cons_ptr cdr;
};


/* Returns a new parse node with NULL car and cdr. */
jz_cons* jz_cons_empty(JZ_STATE);

jz_enum* jz_enum_new(JZ_STATE, jz_byte val);

jz_cons* jz_cons_new(JZ_STATE, jz_tag* car, jz_tag* cdr);

/* Wraps a jz_tvalue in a jz_cons_ptr.
   This pointer should be assigned to the car or cdr of one and only one jz_cons;
   otherwise, it won't be garbage cllected properly.

   Note that this doesn't necessarily return a pointer to a jz_tvalue;
   if the tvalue is itself a pointer to a garbage-collectable object,
   that pointer will be returned instead. */
jz_cons_ptr jz_cons_ptr_wrap(JZ_STATE, jz_tvalue val);

jz_tvalue jz_cons_ptr_unwrap(JZ_STATE, jz_cons_ptr ptr);

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
