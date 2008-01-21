#ifndef JZ_STRING_H
#define JZ_STRING_H

#include <stdbool.h>

#include <unicode/ustring.h>
#include "jazz.h"
#include "value.h"
#include "gc.h"

typedef struct {
  gc_header gc;
  UChar str[1];
} jz_str_value;

struct jz_str {
  gc_header gc;
  int start;
  int length;
  union {
    const UChar* ext;
    jz_str_value* val;
  } value;
};

#define JZ_STR_IS_EXT(str) (JZ_GC_UTAG(str) & 1)

#define JZ_STR_PTR(string)                              \
  ((JZ_STR_IS_EXT(string) ? (string)->value.ext :       \
    (string)->value.val->str) + (string)->start)

#define JZ_STR_INT_PTR(string) \
  (assert(!JZ_STR_IS_EXT(string)), (string)->value.val->str + (string)->start)

/* Creates a new jz_str* from external string data.
   This is shallow,
   so the 'value' member is the same as the 'value' argument. */
jz_str* jz_str_external(JZ_STATE, int length, const UChar* value);

/* Creates a new jz_str*.
   This is deep,
   so the character data is copied out of the 'value' argument
   into a newly allocated 'value' member. */
jz_str* jz_str_deep_new(JZ_STATE, int length, const UChar* value);

/* Allocates space a new jz_str*, but sets the length to 0.
   The UChar* buffer can be accessed with JZ_STR_INT_PTR. */
jz_str* jz_str_alloc(JZ_STATE, int space);

/* Returns a null jz_str*.
   This means that 'length' is 0 and 'value' is NULL.
   The string itself isn't null, though,
   and must be freed when no longer in use. */
jz_str* jz_str_null(JZ_STATE);

/* Creates a shallow duplicate of 'this'.
   This means that both 'this' and the return value
   point to the same character string. */
jz_str* jz_str_dup(JZ_STATE, const jz_str* this);

/* Creates a deep duplicate of 'this'.
   This means that the data of the 'value' member of 'this'
   is copied to a newly allocated 'value' member of the return value. */
jz_str* jz_str_deep_dup(JZ_STATE, const jz_str* this);

jz_str* jz_str_substr(JZ_STATE, const jz_str* this, int start);

/* Returns a shallow substring of 'this',
   beginning at the index 'start'
   and going through the index 'end'.
   This is shallow, so the new string is pointing to
   the same character array as 'this'. */
jz_str* jz_str_substr2(JZ_STATE, const jz_str* this, int start, int end);

/* Shallow */
jz_str* jz_str_strip(JZ_STATE, const jz_str* this);

/* Deep */
jz_str* jz_str_concat(JZ_STATE, const jz_str* s1, const jz_str* s2);

/* Returns whether or not s1 and s2 are equivalent strings. */
bool jz_str_equal(JZ_STATE, const jz_str* s1, const jz_str* s2);

/* Returns an integer indicating
   how s1 and s2 are ordered relative to each other.
   The return value is > 0 if s1 > s2, < 0 if s1 < s2,
   and 0 if s1 == s2.

   The ordering is defined by the ECMAscript spec. */
int jz_str_comp(JZ_STATE, const jz_str* s1, const jz_str* s2);

double jz_str_to_num(JZ_STATE, const jz_str* this);

/* Returns a newly allocated character array containing 'this'
   transcoded into UTF-8. */
char* jz_str_to_chars(JZ_STATE, const jz_str* this);

#define jz_str_from_literal(jz, value)           \
  jz_str_from_chars(jz, value, sizeof(value) - 1)

jz_str* jz_str_from_chars(JZ_STATE, const char* value, int length);

#endif
