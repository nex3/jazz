#include <stdio.h>
#include <assert.h>

#include "object.h"
#include "string.h"
#include "gc.h"
#include "state.h"

#define MASK(obj, hash) ((hash) & ((obj)->capacity - 1))

/* Controls how large objects' hash tables are initially: 2 ** DEFAULT_ORDER. */
#define DEFAULT_ORDER (3)

/* A percentage such that if a hash table is more than this percent full,
   it's re-hashed into a larger table. */
#define LOAD_CAPACITY (75)

/* Each time a table is resized,
   its new capacity is old_capacity * (2 ** ORDER_INCREMENT) */
#define ORDER_INCREMENT (2)

static jz_obj_cell* get_cell(JZ_STATE, jz_obj* this, jz_str* key, bool removed);
static void grow(JZ_STATE, jz_obj* this);

jz_obj* jz_obj_new(JZ_STATE) {
  jz_obj* this = (jz_obj*)jz_gc_malloc(jz, jz_t_obj, sizeof(jz_obj));

  this->capacity = 1 << DEFAULT_ORDER;
  this->order = DEFAULT_ORDER;
  this->size = 0;
  this->table = calloc(sizeof(jz_obj_cell), this->capacity);
  return this;
}

jz_tvalue jz_obj_get(JZ_STATE, jz_obj* this, jz_str* key) {
  jz_obj_cell* cell = get_cell(jz, this, key, false);

  /* TODO: Check prototype */
  /* Handily enough, a missing cell has a nulled value,
     and a fully null tvalue is not-so-coincidentally undefined. */
  return cell->value;
}

void jz_obj_put(JZ_STATE, jz_obj* this, jz_str* key, jz_tvalue val) {
  jz_obj_cell* cell;

  JZ_GC_WRITE_BARRIER(jz, this, key);
  JZ_GC_WRITE_BARRIER_VAL(jz, this, val);

  this->size++;
  if ((this->size * 100)/this->capacity > LOAD_CAPACITY)
    grow(jz, this);

  cell = get_cell(jz, this, key, true);

  cell->value = val;
  cell->key = key;
}

jz_obj_cell* get_cell(JZ_STATE, jz_obj* this,
                      jz_str* key, bool removed) {
  jz_obj_cell* cell;
  /* TODO: uint32 */
  unsigned int hash = jz_str_hash(jz, key);

  hash = MASK(this, hash);
  cell = this->table + hash;

  for (; cell->key != JZ_OBJ_EMPTY_KEY; cell++) {
    if (removed) {
      if (cell->key == JZ_OBJ_REMOVED_KEY)
        return cell;
    }

    if (jz_str_equal(jz, cell->key, key))
      return cell;

    /* Wrap around */
    if (cell == this->table + this->capacity - 1)
      cell = this->table;
  }

  return cell;
}

void grow(JZ_STATE, jz_obj* this) {
  jz_obj_cell* old_table = this->table;
  jz_obj_cell* old_table_end = this->table + this->capacity;

  this->order += ORDER_INCREMENT;
  this->capacity = 1 << this->order;
  this->table = calloc(sizeof(jz_obj_cell), this->capacity);

  for (; old_table < old_table_end; old_table++) {
    if (old_table->key != JZ_OBJ_EMPTY_KEY && old_table->key != JZ_OBJ_REMOVED_KEY)
      jz_obj_put(jz, this, old_table->key, old_table->value);
  }
}

jz_tvalue jz_obj_to_str(JZ_STATE, jz_obj* obj) {
  /* TODO: Replace calls to this with actual calls to toString(). */
  return jz_wrap_str(jz, jz_str_from_literal(jz, "[object Object]"));
}

jz_tvalue jz_obj_value_of(JZ_STATE, jz_obj* obj) {
  /* TODO: Replace calls to this with actual calls to toString(). */
  return jz_wrap_obj(jz, obj);
}

void jz_obj_free(JZ_STATE, jz_obj* obj) {
  free(obj->table);
  free(obj);
}
