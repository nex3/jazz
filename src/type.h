#ifndef JZ_TYPE_H
#define JZ_TYPE_H

#include <stdbool.h>

#include <unicode/ustring.h>

typedef struct {
  unsigned char tag;
  UChar str[1];
} jz_str_value;

typedef struct {
  unsigned char tag;
  int start;
  int length;
  union {
    const UChar* ext;
    jz_str_value* val;
  } value;
} jz_str;

enum {
  /* Giving undefined a 0 flag means that zero-ed memory
     is identified as jz_undef, which saves some manual setting. */
  jz_undef = 0x00,
  jz_num   = 0x01,
  jz_bool  = 0x02,
  jz_strt  = 0x03
} jz_type_type;

typedef union {
  double num;
  jz_str* str;
  bool b;
} jz_value;

typedef struct {
  unsigned char tag;
  jz_value value;
} jz_tvalue;

#endif
