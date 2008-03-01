#ifndef JZ_VALUE_H
#define JZ_VALUE_H

#include <math.h>
#include <float.h>

#include "jazz.h"

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
  /* Giving undefined a 0 flag means that zero-ed memory
     is identified as jz_t_undef, which saves some manual setting. */
  jz_t_undef = 0x00,
  jz_t_int,
  jz_t_bool,

  /* An opaque pointer or value. */
  jz_t_void,

  /* All type flags past this point
     must refer to types with jz_gc_header headers. */
  jz_t_str,
  jz_t_num,
  jz_t_enum, /* Used in parser */
  jz_t_cons,
  jz_t_str_value,
  jz_t_closure_locals,
  jz_t_obj,
  jz_t_proto
} jz_type;

typedef union {
  jz_bool b;
  int i;
  jz_num* num;
  jz_gc_header* gc;
  jz_obj* obj;
  jz_str* str;
  jz_proto* proto;
  void* ptr;
} jz_value;

typedef struct {
  /* The lower four bits are used as property flags
     when storing the value in an object.
     The value of these flags is undefined
     unless otherwise specified by the function returning the tvalue.*/
  jz_tag tag;
  jz_value value;
} jz_tvalue;

typedef enum {
  jz_hint_none,
  jz_hint_number,
  jz_hint_string
} jz_to_primitive_hint;

#define JZ_TAG_TYPE(tag) ((tag) >> 4)
#define JZ_TAG_WITH_TYPE(tag, type) (((tag) & ~0xf0) | ((type) << 4))

#define JZ_TVAL_TYPE(value) JZ_TAG_TYPE((value).tag)
#define JZ_TVAL_SET_TYPE(value, type) \
  ((value).tag = JZ_TAG_WITH_TYPE((value).tag, type))

#define JZ_TAG_CAN_BE_GCED(tag) (JZ_TAG_TYPE(tag) >= jz_t_str)
#define JZ_TVAL_CAN_BE_GCED(val) \
  (JZ_TAG_CAN_BE_GCED((val).tag) && (val).value.gc != NULL)

#define JZ_TVAL_IS_NULL(val) \
  (JZ_TVAL_TYPE(val) == jz_t_obj && (val).value.obj == NULL)

#define JZ_TVAL_IS_PRIMITIVE(val) \
  (JZ_TVAL_TYPE(val) != jz_t_obj || (val).value.obj == NULL)

#define JZ_BITMASK(bit) (1 << (bit))

#define JZ_BIT(field, bit) ((field) & JZ_BITMASK(bit))
#define JZ_SET_BIT(field, bit, val)             \
  (void)((val) ?                                \
         ((field) |= JZ_BITMASK(bit)) :         \
         ((field) &= ~JZ_BITMASK(bit)))

#define JZ_IS_NUM(v) ((JZ_TVAL_TYPE(v) == jz_t_num) || JZ_TVAL_TYPE(v) == jz_t_int)

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

jz_bool jz_values_equal(JZ_STATE, jz_tvalue v1, jz_tvalue v2);
jz_bool jz_values_strict_equal(JZ_STATE, jz_tvalue v1, jz_tvalue v2);
double jz_values_comp(JZ_STATE, jz_tvalue v1, jz_tvalue v2);

#define jz_to_wrapped_num(jz, val) \
  (jz_wrap_num(jz, jz_to_num(jz, val)))
#define jz_to_wrapped_bool(jz, val) \
  (jz_wrap_bool(jz, jz_to_bool(jz, val)))
#define jz_to_wrapped_str(jz, val) \
  (jz_wrap_str(jz, jz_to_str(jz, val)))

jz_tvalue jz_wrap_int(JZ_STATE, int num);
jz_tvalue jz_wrap_num(JZ_STATE, double num);
jz_tvalue jz_wrap_bool(JZ_STATE, jz_bool b);
jz_tvalue jz_wrap_obj(JZ_STATE, jz_obj* obj);
jz_tvalue jz_wrap_str(JZ_STATE, jz_str* str);
jz_tvalue jz_wrap_proto(JZ_STATE, jz_proto* proto);

jz_tvalue jz_wrap_void(JZ_STATE, void* ptr);

double jz_to_num(JZ_STATE, jz_tvalue val);
jz_str* jz_to_str(JZ_STATE, jz_tvalue val);
jz_obj* jz_to_obj(JZ_STATE, jz_tvalue val);
jz_bool jz_to_bool(JZ_STATE, jz_tvalue val);
int jz_to_int32(JZ_STATE, jz_tvalue val);
unsigned int jz_to_uint32(JZ_STATE, jz_tvalue val);
jz_tvalue jz_to_primitive(JZ_STATE, jz_tvalue val,
                          jz_to_primitive_hint hint);

double jz_num_mod(JZ_STATE, jz_tvalue val1, jz_tvalue val2);
jz_str* jz_num_to_str(JZ_STATE, double num);

#endif
