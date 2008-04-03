#include "state.h"
#include "string.h"
#include "parse.h"
#include "compile.h"
#include "vm.h"
#include "core/core.h"
#include "core/global.h"

#include <stdlib.h>
#include <unicode/ustdio.h>

int main(int argc, char** argv) {
  UFILE* ustdin = u_finit(stdin, NULL, NULL);
  jz_state* jz = jz_init();
  char* result;

  jz_init_core(jz);

  result = jz_str_to_chars(jz, jz_to_str(jz, jz_load(jz, ustdin)));
  puts(result);
  free(result);

  u_fclose(ustdin);
  jz_free_state(jz);

  return 0;
}
