#include <stdlib.h>
#include <unicode/ustdio.h>
#include "lex.h"
#include "parse.h"
#include "string.h"

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

  return 0;
}
