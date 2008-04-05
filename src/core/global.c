#include "core/global.h"
#include "state.h"
#include "function.h"
#include "object.h"
#include "vm.h"

#include <stdio.h>

jz_val jz_load(JZ_STATE, UFILE* file) {
  UChar* str = calloc(sizeof(UChar), 2000);
  int32_t len;
  jz_str* input;
  jz_cons* root; 
  jz_bytecode* bytecode;
  jz_val result;

  len = u_file_read(str, 2000, file);
  input = jz_str_external(jz, len, str);
  if (!(root = jz_parse_string(jz, input))) exit(1);
  free(str);
  if (!(bytecode = jz_compile(jz, root))) exit(1);

  result = jz_vm_run(jz, bytecode);
  jz_free_bytecode(jz, bytecode);
  return result;
}

static jz_val load(JZ_STATE, jz_args* args, jz_val arg) {
  char* filename = jz_str_to_chars(jz, jz_to_str(jz, arg));
  UFILE* file = u_fopen(filename, "r", NULL, NULL);
  jz_val result;

  if (file == NULL) {
    fprintf(stderr, "File does not exist: %s\n", filename);
    exit(1);
  }

  free(filename);
  result = jz_load(jz, file);
  u_fclose(file);

  return result;
}

static jz_val is_nan(JZ_STATE, jz_args* args, jz_val arg) {
  double num = jz_to_num(jz, arg);
  return jz_wrap_bool(jz, JZ_NUM_IS_NAN(num));
}

void jz_init_global(JZ_STATE) {
  jz_obj* obj = jz->global_obj;

  jz_def(jz, obj, "isNaN", is_nan, 1);
  jz_def(jz, obj, "load", load, 1);
}
