#ifndef JZ_JAZZ_H
#define JZ_JAZZ_H

#include <assert.h>

/* Prototype structs that would cause ciruclar-reference problems otherwise. */ 
typedef struct jz_state jz_state;
typedef struct jz_obj jz_obj;
typedef struct jz_str jz_str;
typedef struct jz_proto jz_proto;
typedef struct jz_gc_header jz_gc_header;

#define JZ_STATE jz_state* jz

#ifdef NDEBUG
#define jz_long_assert(expr) assert(expr)
#else

/* This is occasionally neccessary because C90
   limits the number of chars in a literal string,
   which means that normal assertions can only be so long. */
#define jz_long_assert(expr) \
  ((expr) ? ((void)NULL) : assert(0 && "Jazz long assertion failed!"))

#endif

#endif
