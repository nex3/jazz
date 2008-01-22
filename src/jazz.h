#ifndef JZ_JAZZ_H
#define JZ_JAZZ_H

/* Prototype structs that would cause ciruclar-reference problems otherwise. */ 
typedef struct jz_state jz_state;
typedef struct jz_str jz_str;
typedef struct jz_gc_header jz_gc_header;

#define JZ_STATE jz_state* jz

#endif
