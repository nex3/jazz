#ifndef JZ_VECTOR_H
#define JZ_VECTOR_C

#include <string.h>
#include <assert.h>

/* This is a poor substitute for generics, but so be it. */

#define JZ_VECTOR_CAPACITY_MULT 1.5

#define JZ_DECLARE_VECTOR(type)                                         \
  typedef struct {                                                      \
    size_t capacity;                                                    \
    type* next;                                                         \
    type* values;                                                       \
  } type ## _vector;                                                    \
                                                                        \
  type ## _vector* type ## _vector_new();                               \
  void type ## _vector_append(type ## _vector* vector, type val);       \
  size_t type ## _vector_compress(type ## _vector* vector);             \
                                                                        \
  void type ## _vector_resize(type ## _vector* vector);                 \

#define JZ_DEFINE_VECTOR(type, initial)                                 \
                                                                        \
  type ## _vector* type ## _vector_new() {                              \
    type ## _vector* vector = malloc(sizeof(type ## _vector));          \
    vector->next = vector->values = calloc(sizeof(type), initial);      \
    vector->capacity = initial;                                         \
    return vector;                                                      \
  }                                                                     \
                                                                        \
  void type ## _vector_append(type ## _vector* vector, type val) {      \
    if (vector->next - vector->values == vector->capacity)              \
      type ## _vector_resize(vector);                                   \
    assert(vector->next - vector->values < vector->capacity);           \
                                                                        \
    *(vector->next) = val;                                              \
    vector->next++;                                                     \
  }                                                                     \
                                                                        \
  void type ## _vector_resize(type ## _vector* vector) {                \
    type* old_values = vector->values;                                  \
    type* old_next = vector->next;                                      \
    size_t old_capacity = vector->capacity;                             \
                                                                        \
    vector->capacity *= JZ_VECTOR_CAPACITY_MULT;                        \
    vector->values = calloc(sizeof(type), vector->capacity);            \
    memcpy(vector->values, old_values, old_capacity);                   \
    vector->next = vector->values + (old_next - old_values);            \
    free(old_values);                                                   \
  }                                                                     \

#endif
