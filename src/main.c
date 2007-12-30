#include "lex.h"
#include "parse.h"
#include "string.h"
#include "compile.h"
#include "vm.h"
#include "type.h"

#include <stdlib.h>
#include <unicode/ustdio.h>

int main(int argc, char** argv) {
  UFILE* ustdin = u_finit(stdin, NULL, NULL);
  UChar* str = calloc(sizeof(UChar), 2000);
  int32_t len;

  len = u_file_read(str, 2000, ustdin);

  jz_lex_init();
  u_fclose(ustdin);

  {
    jz_str* input = jz_str_new(len, str);
    jz_parse_node* root; 
    jz_bytecode* bytecode;

    root = jz_parse_string(input);
    if (!root) exit(1);

    free(str);
    free(input);

    bytecode = jz_compile(root);
    if (!bytecode) exit(1);

    printf("%f\n", jz_to_num(jz_vm_run(bytecode)));

    free(bytecode->code);
    free(bytecode);
  }

  return 0;
}
