#ifndef JZ_OBJECT_H
#define JZ_OBJECT_H

#include "jazz.h"
#include "gc.h"
#include "value.h"
#include "string.h"
#include "function.h"

typedef struct {
  jz_str* key; /* NULL when the cell is empty. */
  jz_tvalue value;
} jz_obj_cell;

/* jz_obj uses Cuckoo Hashing to store and retrieve key-value pairs.
   See http://citeseer.ist.psu.edu/pagh01cuckoo.html. */
struct jz_obj {
  jz_gc_header gc;
  jz_proto* prototype;
  jz_fn* call;
  void* data;
  /* TODO: uint32 */
  unsigned int capacity; /* The number of cells in the table. */
  jz_byte order; /* 1 << capacity */
  unsigned int size; /* The number of active elements currently in the table. */
  jz_obj_cell* table;
};

#define JZ_OBJ_EMPTY_KEY   ((jz_str*)0)
#define JZ_OBJ_REMOVED_KEY ((jz_str*)1)

jz_obj* jz_obj_new(JZ_STATE);
jz_obj* jz_obj_new_bare(JZ_STATE);

jz_tvalue jz_obj_get(JZ_STATE, jz_obj* this, jz_str* key);
#define jz_obj_get_ptr(jz, this, key)           \
  ((void*)jz_obj_get(jz, this, key).value.ptr)
#define jz_obj_get2(jz, this, key)              \
  jz_obj_get(jz, this, jz_str_from_literal(jz, key))

void jz_obj_put(JZ_STATE, jz_obj* this, jz_str* key, jz_tvalue val);
#define jz_obj_put_ptr(jz, this, key, val)              \
  jz_obj_put(jz, this, key, jz_wrap_void(jz, (val)))
#define jz_obj_put2(jz, this, key, val)                         \
  jz_obj_put(jz, this, jz_str_from_literal(jz, key), val)

jz_bool jz_obj_remove(JZ_STATE, jz_obj* this, jz_str* key);

#define jz_obj_null(jz) ((jz_obj*)NULL)
#define jz_obj_is_null(jz, obj) ((obj) == NULL)

jz_tvalue jz_obj_to_str(JZ_STATE, jz_obj* obj);
jz_tvalue jz_obj_value_of(JZ_STATE, jz_obj* obj);

void jz_obj_free(JZ_STATE, jz_obj* obj);

void jz_init_obj_proto(JZ_STATE);

#endif
