#ifndef JZ_OBJECT_H
#define JZ_OBJECT_H

enum {
  jz_num
};

typedef union {
  double num;
} jz_value;

typedef struct {
  unsigned char type;
  jz_value value;
} jz_tvalue;

jz_tvalue jz_wrap_num(double num);

#endif
