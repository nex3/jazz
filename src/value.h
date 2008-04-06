#ifndef JZ_VALUE_H
#define JZ_VALUE_H

#include <math.h>
#include <float.h>

#include "jazz.h"

typedef enum {
  jz_tt_ptr,
  jz_tt_const,
  jz_tt_void,
  jz_tt_int
} jz_tag_type;

typedef enum {
  jz_ct_false,
  jz_ct_true,
  jz_ct_undef
} jz_const_type;

typedef void* jz_val;

/* The integral type that jz_val is converted to
   to do bit-twiddling and so forth.

   TODO: Really an unsigned int? */
typedef unsigned int jz_ival;

/* This tag stores type information
   and various other sorts of metadata
   about a value.

   The uppder four bits (bits 4, 5, 6, and 7)
   are always type information (jz_type),
   and are accessed via JZ_TAG_TYPE.

   The use of the lower four bits (bits 0, 1, 2, and 3)
   varies depending on the context. */
typedef jz_byte jz_tag;

typedef enum {
  jz_t_str,
  jz_t_num,
  jz_t_enum, /* Used in parser */
  jz_t_cons,
  jz_t_str_value,
  jz_t_closure_locals,
  jz_t_obj,
  jz_t_proto,
  jz_t_bytecode,

  /* Non-GCable types */
  jz_t_void,
  jz_t_undef,
  jz_t_bool,
  jz_t_int
} jz_type;

struct jz_gc_header {
  /* The first two bits (0 and 1) of this tag
     are reserved for the GC's internal use.
     The second two (2 and 3) may be used by individual structs
     for any tagging they need. */
  jz_tag tag;
  jz_gc_header* next;
};

typedef enum {
  jz_hint_none,
  jz_hint_number,
  jz_hint_string
} jz_to_primitive_hint;

#define JZ_TAG_TYPE(tag) ((tag) >> 4)
#define JZ_TAG_WITH_TYPE(tag, type) (((tag) & ~0xf0) | ((type) << 4))

#define JZ_VAL_TAG(value) (((jz_ival)(value)) & 3)
#define JZ_VAL_TYPE(value)                              \
  ((value) == NULL                  ? jz_t_obj     :    \
   JZ_VAL_TAG(value) == jz_tt_void  ? jz_t_void    :    \
   JZ_VAL_TAG(value) == jz_tt_int   ? jz_t_int     :    \
   (value) == JZ_UNDEFINED          ? jz_t_undef   :    \
   (value) == JZ_TRUE ||                                \
   (value) == JZ_FALSE              ? jz_t_bool    :    \
   JZ_TAG_TYPE(((jz_gc_header*)value)->tag))

#define JZ_IS_GC_TYPE(val, type)                       \
  (val != NULL && JZ_VAL_TAG(val) == jz_tt_ptr &&      \
   JZ_TAG_TYPE(((jz_gc_header*)val)->tag) == type)

#define JZ_VAL_CAN_BE_GCED(val) \
  (!JZ_VAL_IS_NULL(val) && JZ_VAL_TAG(val) == jz_tt_ptr)

#define JZ_VAL_IS_CONST(val) (JZ_VAL_TAG(val) == jz_tt_const)
#define JZ_VAL_IS_NULL(val) ((val) == NULL)

#define JZ_VAL_IS_PRIMITIVE(val) \
  (JZ_VAL_TYPE(val) != jz_t_obj || JZ_VAL_IS_NULL(val))

#define JZ_BITMASK(bit) (1 << (bit))

#define JZ_BIT(field, bit) ((field) & JZ_BITMASK(bit))
#define JZ_SET_BIT(field, bit, val)             \
  (void)((val) ?                                \
         ((field) |= JZ_BITMASK(bit)) :         \
         ((field) &= ~JZ_BITMASK(bit)))

#define JZ_TYPE_IS_NUM(t) (t == jz_t_num || t == jz_t_int)
#define JZ_IS_NUM(v) (JZ_TYPE_IS_NUM(JZ_VAL_TYPE(v)))

#define JZ_INT_MAX ((1 << ((sizeof(void*) * 8) - 3)) - 1)
#define JZ_INT_MIN (-(1 << ((sizeof(void*) * 8) - 3)))

#define JZ_UNDEFINED ((jz_val)((jz_ct_undef << 2) + jz_tt_const))
#define JZ_TRUE      ((jz_val)((jz_ct_true  << 2) + jz_tt_const))
#define JZ_FALSE     ((jz_val)((jz_ct_false << 2) + jz_tt_const))

#define JZ_NEG_0   (-0.0)
#define JZ_INF     (1.0/0.0)
#define JZ_NEG_INF (-1.0/0.0)
#define JZ_NAN     (0.0/0.0)

#define JZ_NUM_IS_NAN(num)         ((num) != (num))
#define JZ_NUM_IS_INF(num)         ((num) == (num) + 1)
#define JZ_NUM_IS_POS_0(num)       (1.0/(num) == JZ_INF)
#define JZ_NUM_IS_NEG_0(num)       (1.0/(num) == JZ_NEG_INF)
#define JZ_NUM_WITHIN_EPS(num)     ((num) < DBL_EPSILON && (num) > -DBL_EPSILON)
#define JZ_NUMS_WITHIN_EPS(n1, n2) (JZ_NUM_WITHIN_EPS(n1 - n2))
#define JZ_NUM_IS_INT(num)         (JZ_NUMS_WITHIN_EPS(num, floor(num)))

jz_bool jz_values_equal(JZ_STATE, jz_val v1, jz_val v2);
jz_bool jz_values_strict_equal(JZ_STATE, jz_val v1, jz_val v2);
double jz_values_comp(JZ_STATE, jz_val v1, jz_val v2);

#define jz_to_wrapped_num(jz, val) \
  (jz_wrap_num(jz, jz_to_num(jz, val)))
#define jz_to_wrapped_bool(jz, val) \
  (jz_wrap_bool(jz, jz_to_bool(jz, val)))

jz_val jz_wrap_int(JZ_STATE, int num);
jz_val jz_wrap_num(JZ_STATE, double num);
#define jz_wrap_bool(jz, b) ((jz_val)((((b) ? 1 : 0) << 2) + jz_tt_const))
#define jz_wrap_void(jz, ptr) ((jz_val)(((jz_ival)(ptr)) | jz_tt_void))

#define jz_unwrap_void(jz, val) ((void*)(((jz_ival)(val)) & ~jz_tt_void))

double jz_to_num(JZ_STATE, jz_val val);
jz_str* jz_to_str(JZ_STATE, jz_val val);
jz_obj* jz_to_obj(JZ_STATE, jz_val val);
jz_bool jz_to_bool(JZ_STATE, jz_val val);
int jz_to_int32(JZ_STATE, jz_val val);
unsigned int jz_to_uint32(JZ_STATE, jz_val val);
jz_val jz_to_primitive(JZ_STATE, jz_val val,
                          jz_to_primitive_hint hint);

double jz_num_mod(JZ_STATE, jz_val val1, jz_val val2);
jz_str* jz_num_to_str(JZ_STATE, double num);

#endif
