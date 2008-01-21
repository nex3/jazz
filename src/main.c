#include "lex.h"
#include "parse.h"
#include "string.h"
#include "compile.h"
#include "vm.h"
#include "value.h"
#include "state.h"

#include <stdlib.h>
#include <unicode/ustdio.h>

int main(int argc, char** argv) {
  UFILE* ustdin = u_finit(stdin, NULL, NULL);
  UChar* str = calloc(sizeof(UChar), 2000);
  int32_t len;

  len = u_file_read(str, 2000, ustdin);
  u_fclose(ustdin);

  {
    jz_state* jz = jz_init();
    jz_str* input = jz_str_external(jz, len, str);
    jz_parse_node* root; 
    jz_bytecode* bytecode;

    root = jz_parse_string(jz, input);
    if (!root) exit(1);

    free(str);
    free(input);

    bytecode = jz_compile(jz, root);
    if (!bytecode) exit(1);

    jz_free_parse_tree(jz, root);

    printf("%s\n", jz_str_to_chars(jz, jz_to_str(jz, jz_vm_run(jz, bytecode))));

    jz_free_bytecode(jz, bytecode);
  }

  return 0;
}
