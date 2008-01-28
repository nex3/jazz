#ifndef JZ_OBJECT_H
#define JZ_OBJECT_H

#include "jazz.h"
#include "gc.h"
#include "value.h"

typedef struct {
  jz_str* key; /* NULL when the cell is empty. */
  jz_tvalue value;
} jz_obj_cell;

/* jz_obj uses Cuckoo Hashing to store and retrieve key-value pairs.
   See http://citeseer.ist.psu.edu/pagh01cuckoo.html. */
struct jz_obj {
  jz_gc_header gc;
  /* TODO: uint32 */
  unsigned int capacity; /* The number of cells in the table. */
  unsigned char order; /* 1 << capacity */
  unsigned int size; /* The number of active elements currently in the table. */
  jz_obj_cell* table;
};

jz_obj* jz_obj_new(JZ_STATE);

jz_tvalue jz_obj_get(JZ_STATE, jz_obj* this, jz_str* key);
void jz_obj_put(JZ_STATE, jz_obj* this, jz_str* key, jz_tvalue val);

#define jz_obj_null(jz) ((jz_obj*)NULL)
#define jz_obj_is_null(jz, obj) ((obj) == NULL)

jz_tvalue jz_obj_to_str(JZ_STATE, jz_obj* obj);
jz_tvalue jz_obj_value_of(JZ_STATE, jz_obj* obj);

#endif
