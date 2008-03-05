#include <stdio.h>
#include <assert.h>

#include "object.h"
#include "string.h"
#include "gc.h"
#include "state.h"
#include "prototype.h"

#define MASK(obj, hash) ((hash) & ((obj)->capacity - 1))

/* Controls how large objects' hash tables are initially: 2 ** DEFAULT_ORDER. */
#define DEFAULT_ORDER (3)

/* A percentage such that if a hash table is more than this percent full,
   it's re-hashed into a larger table. */
#define LOAD_CAPACITY (75)

/* Each time a table is resized,
   its new capacity is old_capacity * (2 ** ORDER_INCREMENT) */
#define ORDER_INCREMENT (2)

static jz_obj_cell* get_cell(JZ_STATE, jz_obj* this, jz_str* key, jz_bool removed);
static void grow(JZ_STATE, jz_obj* this);

jz_obj* jz_obj_new(JZ_STATE) {
  /* Integrate with [[Constructor]] */
  jz_obj* obj = jz_inst(jz, "Object");

  /* Note: the prototype property of a plain object
     is not specified by ECMAscript. */
  jz_obj_put2(jz, obj, "prototype", obj->prototype->obj);

  return obj;
}

jz_obj* jz_obj_new_bare(JZ_STATE) {
  jz_obj* this = (jz_obj*)jz_gc_malloc(jz, jz_t_obj, sizeof(jz_obj));

  this->capacity = 1 << DEFAULT_ORDER;
  this->order = DEFAULT_ORDER;
  this->size = 0;
  this->table = calloc(sizeof(jz_obj_cell), this->capacity);
  this->prototype = NULL;
  this->call = NULL;

  return this;
}

jz_val jz_obj_get(JZ_STATE, jz_obj* this, jz_str* key) {
  jz_obj_cell* cell = get_cell(jz, this, key, jz_false);

  if (cell->key == JZ_OBJ_EMPTY_KEY) {
    if (this->prototype != NULL)
      return jz_obj_get(jz, this->prototype->obj, key);
    else
      return JZ_UNDEFINED;
  }

  return cell->value;
}

void* jz_obj_get_ptr(JZ_STATE, jz_obj* this, jz_str* key) {
  jz_val val = jz_obj_get(jz, this, key);

  if (val == JZ_UNDEFINED)
    return NULL;

  return jz_unwrap_void(jz, val);
}

void jz_obj_put(JZ_STATE, jz_obj* this, jz_str* key, jz_val val) {
  jz_obj_cell* cell;

  JZ_GC_WRITE_BARRIER(jz, this, key);
  JZ_GC_WRITE_BARRIER_VAL(jz, this, val);

  this->size++;
  if ((this->size * 100)/this->capacity > LOAD_CAPACITY)
    grow(jz, this);

  cell = get_cell(jz, this, key, jz_true);

  cell->value = val;
  cell->key = key;
}

void* jz_obj_remove_ptr(JZ_STATE, jz_obj* this, jz_str* key, jz_bool* found) {
  jz_val val = jz_obj_remove(jz, this, key, found);

  if (val == JZ_UNDEFINED)
    return NULL;

  return jz_unwrap_void(jz, val);
}

jz_val jz_obj_remove(JZ_STATE, jz_obj* this, jz_str* key, jz_bool* found) {
  jz_obj_cell* cell = get_cell(jz, this, key, jz_false);

  if (cell->key != JZ_OBJ_EMPTY_KEY) {
    if (found != NULL)
      *found = jz_true;

    cell->key = JZ_OBJ_REMOVED_KEY;
    return cell->value;
  } else {
    if (found != NULL)
      *found = jz_false;

    return JZ_UNDEFINED;
  }
}

void jz_obj_each(JZ_STATE, jz_obj* this, jz_obj_fn* fn, void* data) {
  jz_obj_cell* cell = this->table;
  jz_obj_cell* top = this->table + this->capacity;

  for (; cell < top; cell++) {
    if (cell->key != JZ_OBJ_EMPTY_KEY &&
        cell->key != JZ_OBJ_REMOVED_KEY)
      fn(jz, cell->key, cell->value, data);
  }
}

jz_obj_cell* get_cell(JZ_STATE, jz_obj* this,
                      jz_str* key, jz_bool removed) {
  jz_obj_cell* cell;
  /* TODO: uint32 */
  unsigned int hash = jz_str_hash(jz, key);

  hash = MASK(this, hash);
  cell = this->table + hash;

  for (; cell->key != JZ_OBJ_EMPTY_KEY; cell++) {
    if (cell->key == JZ_OBJ_REMOVED_KEY) {
      if (removed)
        return cell;
    } else {
      if (jz_str_equal(jz, cell->key, key))
        return cell;
    }

    /* Wrap around */
    if (cell == this->table + this->capacity - 1)
      cell = this->table;
  }

  return cell;
}

void grow(JZ_STATE, jz_obj* this) {
  jz_obj_cell* old_table = this->table;
  jz_obj_cell* old_table_iter = old_table;
  jz_obj_cell* old_table_end = this->table + this->capacity;

  this->order += ORDER_INCREMENT;
  this->capacity = 1 << this->order;
  this->table = calloc(sizeof(jz_obj_cell), this->capacity);

  for (; old_table_iter < old_table_end; old_table_iter++) {
    if (old_table_iter->key != JZ_OBJ_EMPTY_KEY &&
        old_table_iter->key != JZ_OBJ_REMOVED_KEY)
      jz_obj_put(jz, this, old_table_iter->key, old_table_iter->value);
  }

  free(old_table);
}

jz_val jz_obj_to_str(JZ_STATE, jz_obj* obj) {
  /* TODO: Replace calls to this with actual calls to toString(). */
  return jz_str_from_literal(jz, "[object Object]");
}

jz_val jz_obj_value_of(JZ_STATE, jz_obj* obj) {
  /* TODO: Replace calls to this with actual calls to toString(). */
  return obj;
}

void jz_obj_free(JZ_STATE, jz_obj* obj) {
  free(obj->table);
  obj->table = NULL;

  if (obj->prototype && obj->prototype->finalizer)
    obj->prototype->finalizer(jz, obj);

  free(obj);
}

void jz_init_obj_proto(JZ_STATE) {
  jz_proto_new(jz, "Object");
}
