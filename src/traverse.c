#include <stdarg.h>

#include "traverse.h"
#include "_cons.h"

static void traverse_several(JZ_STATE, jz_cons* tree, jz_traverse_fn* fn, void* data,
                             int* enum_vals, size_t enum_vals_size);

void jz_traverse_several(JZ_STATE, jz_cons* tree, jz_traverse_fn* fn, void* data,
                         int argc, ...) {
  int* vals = calloc(sizeof(int), argc);
  va_list args;
  int i;

  va_start(args, argc);
  for (i = 0; i < argc; i++)
    vals[i] = va_arg(args, int);
  va_end(args);

  traverse_several(jz, tree, fn, data, vals, argc);
  free(vals);
}


void traverse_several(JZ_STATE, jz_cons* tree, jz_traverse_fn* fn, void* data,
                      int* enum_vals, size_t enum_vals_size) {
  jz_val car;
  jz_val cdr;

  if (tree == NULL)
    return;

  car = CAR(tree);
  cdr = CDR(tree);

  if (car != NULL) {
    jz_byte type = TYPE(car);

    if (type == jz_t_cons)
      traverse_several(jz, NODE(car), fn, data, enum_vals, enum_vals_size);
    else if (type == jz_t_enum) {
      int val = ENUM(car);
      int i;

      for (i = 0; i < enum_vals_size; i++) {
        if (val == enum_vals[i]) {
          if (fn(jz, tree, data))
            break;
          else
            return;
        }
      }
    }
  }

  if (cdr != NULL && TYPE(cdr) == jz_t_cons)
    traverse_several(jz, NODE(cdr), fn, data, enum_vals, enum_vals_size);
}
