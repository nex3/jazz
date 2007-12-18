#include <stdlib.h>
#include <unicode/ustdio.h>
#include "lex.h"
#include "string.h"

int main(int argc, char** argv) {
  UFILE* ustdin = u_finit(stdin, NULL, NULL);
  UChar str[2000];
  int32_t len = u_file_read(str, 2000, ustdin);
  jz_str input = jz_str_new(len, str);
  jz_lex_init(input);

  jz_token_type token;
  while ((token = yylex())) {
    printf("Token: %d\n", token);
  }
  return 0;
}
