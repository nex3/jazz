#include <stdlib.h>
#include <unicode/ustdio.h>
#include "lex.h"
#include "parse.h"
#include "string.h"

int main(int argc, char** argv) {
  jz_lex_init();

  UFILE* ustdin = u_finit(stdin, NULL, NULL);
  UChar str[2000];
  int32_t len = u_file_read(str, 2000, ustdin);
  u_fclose(ustdin);
  jz_str input = jz_str_new(len, str);

  jz_parse_node* root = jz_parse_string(input);
  return 0;
}
