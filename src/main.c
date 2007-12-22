#include "lex.h"
#include "parse.h"
#include "string.h"
#include "vm.h"

#include <stdlib.h>
#include <unicode/ustdio.h>

int main(int argc, char** argv) {
  UFILE* ustdin = u_finit(stdin, NULL, NULL);
  UChar str[2000];
  int32_t len;

  len = u_file_read(str, 2000, ustdin);

  jz_lex_init();
  u_fclose(ustdin);

  {
    jz_str input = jz_str_new(len, str);
    jz_parse_string(input);
  }

  {
    jz_opcode code[] = {
      jz_oc_push_double,
      0, 0, 0, 0, 0, 0, 0, 0,
      jz_oc_push_double,
      0, 0, 0, 0, 0, 0, 0, 0,
      jz_oc_add,
      jz_oc_ret
    };
    *((double*)(code + 1)) = 12.0;
    *((double*)(code + 10)) = 8.0;
    printf("%f\n", jz_vm_run(code, 2));
  }

  return 0;
}
