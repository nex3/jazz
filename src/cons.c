#include <stdarg.h>
#include <stdio.h>

#include "_cons.h"
#include "parse.h"
#include "state.h"
#include "string.h"

static void print_parse_list(JZ_STATE, jz_cons* node, jz_bool start);
static void print_leaf(JZ_STATE, jz_val leaf);

jz_cons* jz_cons_empty(JZ_STATE) {
  jz_cons* to_ret = (jz_cons*)jz_gc_malloc(jz, jz_t_cons, sizeof(jz_cons));

  to_ret->car = to_ret->cdr = NULL;
  return to_ret;
}

jz_enum* jz_enum_new(JZ_STATE, jz_byte val) {
  jz_enum* to_ret = (jz_enum*)jz_gc_malloc(jz, jz_t_enum, sizeof(jz_enum));

  to_ret->val = val;
  return to_ret;
}

jz_cons* jz_cons_new(JZ_STATE, jz_val car, jz_val cdr) {
  jz_cons* to_ret = jz_cons_empty(jz);

  to_ret->car = car;
  to_ret->cdr = cdr;
  return to_ret;
}

jz_cons* jz_list(JZ_STATE, int argc, ...) {
  jz_cons* to_ret;
  jz_cons* end;
  va_list args;
  int i;

  if (argc == 0)
    return NULL;

  va_start(args, argc);

  to_ret = jz_cons_empty(jz);
  end = to_ret;
  to_ret->car = va_arg(args, jz_val);

  for (i = 1; i < argc; i++) {
    jz_cons* next = jz_cons_empty(jz);

    end->cdr = next;
    end = next;
    end->car = va_arg(args, jz_val);
  }

  va_end(args);

  end->cdr = NULL;

  return to_ret;
}

jz_cons* jz_list_concat(JZ_STATE, jz_cons* list1, jz_cons* list2) {
  jz_cons* next = list1;

  if (list1 == NULL) return list2;

  while (next->cdr != NULL) next = NODE(CDR(next));
  next->cdr = list2;

  return list1;
}


jz_cons* jz_list_reverse(JZ_STATE, jz_cons* head) {
  jz_cons *prev = NULL, *curr = NULL, *next = NULL;

  if (head == NULL) return NULL;

  curr = head;
  next = NODE(CDR(curr));

  while (next != NULL) {
    curr->cdr = prev;

    prev = curr;
    curr = next;
    next = NODE(CDR(next));
  }

  curr->cdr = prev;

  return curr;
}


void jz_cons_print(JZ_STATE, jz_cons* root) {
  print_parse_list(jz, root, jz_true);
  putchar('\n');
}

static void print_parse_list(JZ_STATE, jz_cons* node, jz_bool start) {
  if (start)
    putchar('(');

  print_leaf(jz, CAR(node));

  if (node->cdr == NULL)
    putchar(')');
  else if (TYPE(CDR(node)) == jz_t_cons) {
    putchar(' ');
    print_parse_list(jz, NODE(CDR(node)), jz_false);
  } else {
    printf(" . ");
    print_leaf(jz, CDR(node));
    putchar(')');
  }
}

static void print_leaf(JZ_STATE, jz_val leaf) {
  switch (JZ_VAL_TYPE(leaf)) {
  case jz_t_enum:
    printf("%s", jz_parse_names[((jz_enum*)leaf)->val]);
    break;

  case jz_t_cons:
    print_parse_list(jz, (jz_cons*)leaf, jz_true);
    break;

  case jz_t_void:
    printf("<void>");
    break;

  default: {
    char* str = jz_str_to_chars(jz, jz_to_str(jz, leaf));
    printf("%s", str);
    free(str);
  }
  }
}

