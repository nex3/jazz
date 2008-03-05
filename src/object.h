#ifndef JZ_OBJECT_H
#define JZ_OBJECT_H

#include "jazz.h"
#include "gc.h"
#include "value.h"
#include "string.h"
#include "function.h"

typedef struct {
  jz_str* key; /* NULL when the cell is empty. */
  jz_val value;
} jz_obj_cell;

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

typedef void jz_obj_fn(JZ_STATE, jz_str* key, jz_val val, void* data);

#define JZ_OBJ_EMPTY_KEY   ((jz_str*)0)
#define JZ_OBJ_REMOVED_KEY ((jz_str*)1)

jz_obj* jz_obj_new(JZ_STATE);
jz_obj* jz_obj_new_bare(JZ_STATE);

jz_val jz_obj_get(JZ_STATE, jz_obj* this, jz_str* key);
void* jz_obj_get_ptr(JZ_STATE, jz_obj* this, jz_str* key);
#define jz_obj_get2(jz, this, key)              \
  jz_obj_get(jz, this, jz_str_from_literal(jz, key))

void jz_obj_put(JZ_STATE, jz_obj* this, jz_str* key, jz_val val);
#define jz_obj_put_ptr(jz, this, key, val)              \
  jz_obj_put(jz, this, key, jz_wrap_void(jz, (val)))
#define jz_obj_put2(jz, this, key, val)                         \
  jz_obj_put(jz, this, jz_str_from_literal(jz, key), val)

void* jz_obj_remove_ptr(JZ_STATE, jz_obj* this, jz_str* key, jz_bool* found);
jz_val jz_obj_remove(JZ_STATE, jz_obj* this, jz_str* key, jz_bool* found);
void jz_obj_each(JZ_STATE, jz_obj* this, jz_obj_fn* fn, void* data);

#define jz_obj_null(jz) ((jz_obj*)NULL)
#define jz_obj_is_null(jz, obj) ((obj) == NULL)

jz_val jz_obj_to_str(JZ_STATE, jz_obj* obj);
jz_val jz_obj_value_of(JZ_STATE, jz_obj* obj);

void jz_obj_free(JZ_STATE, jz_obj* obj);

void jz_init_obj_proto(JZ_STATE);

#endif
