#ifndef JZ_STRING_H
#define JZ_STRING_H

#include <unicode/ustring.h>

#include <stdbool.h>

/* The Jazz string struct and associated functions.
   jz_strs aren't null-terminated;
   rather, they're paired with their length.
   jz_strs are typically manipulated as pointers. */

typedef struct {
  int length;
  const UChar* value;
} jz_str;

/* Declare a stack-allocated jz_str* with stack-allocated content.
   Produces a statement. */
#define JZ_STR_DECLARE(varname, literal)                        \
  jz_str* varname;                                              \
                                                                \
  {                                                             \
    jz_str strval;                                              \
    UChar buffer[sizeof(literal) - 1];                          \
    UErrorCode error = U_ZERO_ERROR;                            \
                                                                \
    u_strFromUTF8(buffer, sizeof(literal) - 1, &strval.length,  \
                  literal, sizeof(literal) - 1, &error);        \
                                                                \
    if (U_FAILURE(error)) {                                     \
      fprintf(stderr, "ICU Error: %s\n", u_errorName(error));   \
      exit(1);                                                  \
    }                                                           \
                                                                \
    strval.value = buffer;                                      \
                                                                \
    varname = &strval;                                          \
  }

/* Creates a new jz_str*.
   This is shallow,
   so the 'value' member is the same as the 'value' argument. */
jz_str* jz_str_new(int length, const UChar* value);

/* Creates a new jz_str*.
   This is deep,
   so the character data is copied out of the 'value' argument
   into a newly allocated 'value' member. */
jz_str* jz_str_deep_new(int length, const UChar* value);

/* Returns a null jz_str*.
   This means that 'length' is 0 and 'value' is NULL.
   The string itself isn't null, though,
   and must be freed when no longer in use. */
jz_str* jz_str_null();

/* Creates a shallow duplicate of 'this'.
   This means that both 'this' and the return value
   point to the same character string. */
jz_str* jz_str_dup(const jz_str* this);

/* Creates a deep duplicate of 'this'.
   This means that the data of the 'value' member of 'this'
   is copied to a newly allocated 'value' member of the return value. */
jz_str* jz_str_deep_dup(const jz_str* this);

/* Returns a shallow substring of 'this',
   beginning at the index 'start'
   and going through the index 'end'.
   This is shallow, so the new string is pointing to
   the same character array as 'this'. */
jz_str* jz_str_substr(const jz_str* this, int start, int end);

/* Shallow */
jz_str* jz_str_strip(const jz_str* this);

/* Deep */
jz_str* jz_str_concat(const jz_str* s1, const jz_str* s2);

/* Returns whether or not s1 and s2 are equivalent strings. */
bool    jz_str_equal(const jz_str* s1, const jz_str* s2);

/* Returns an integer indicating
   how s1 and s2 are ordered relative to each other.
   The return value is > 0 if s1 > s2, < 0 if s1 < s2,
   and 0 if s1 == s2.

   The ordering is defined by the ECMAscript spec. */
int     jz_str_comp(const jz_str* s1, const jz_str* s2);

double  jz_str_to_num(const jz_str* this);

/* Returns a newly allocated character array containing 'this'
   transcoded into UTF-8. */
char*   jz_str_to_chars(const jz_str* this);

#define jz_str_from_literal(value) \
  jz_str_from_chars(value, sizeof(value) - 1)

jz_str* jz_str_from_chars(const char* value, int length);

#endif
