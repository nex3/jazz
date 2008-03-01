#ifndef JZ_JAZZ_H
#define JZ_JAZZ_H

typedef unsigned char jz_byte;

typedef jz_byte jz_bool;
#define jz_true  ((jz_bool)1)
#define jz_false ((jz_bool)0)

/* Prototype structs that would cause ciruclar-reference problems otherwise. */ 
typedef struct jz_state jz_state;
typedef struct jz_num jz_num;
typedef struct jz_obj jz_obj;
typedef struct jz_str jz_str;
typedef struct jz_proto jz_proto;
typedef struct jz_gc_header jz_gc_header;

#define JZ_STATE jz_state* jz

#endif
