#include <stdarg.h>
#include <stdio.h>

#include "_cons.h"
#include "parse.h"
#include "state.h"
#include "string.h"

static void print_parse_list(JZ_STATE, jz_cons* node, jz_bool start);
static void print_parse_ptr(JZ_STATE, jz_cons_ptr ptr);

jz_cons* jz_cons_empty(JZ_STATE) {
  jz_cons* to_ret = (jz_cons*)jz_gc_malloc(jz, jz_t_cons, sizeof(jz_cons));

  to_ret->car.node = to_ret->cdr.node = NULL;
  return to_ret;
}

jz_enum* jz_enum_new(JZ_STATE, jz_byte val) {
  jz_enum* to_ret = (jz_enum*)jz_gc_malloc(jz, jz_t_enum, sizeof(jz_enum));

  to_ret->val = val;
  return to_ret;
}

jz_cons* jz_cons_new(JZ_STATE, jz_tag* car, jz_tag* cdr) {
  jz_cons* to_ret = jz_cons_empty(jz);

  to_ret->car.tag = car;
  to_ret->cdr.tag = cdr;
  return to_ret;
}

jz_cons_ptr jz_cons_ptr_wrap(JZ_STATE, jz_tvalue val) {
  jz_cons_ptr to_ret;

  if (JZ_TVAL_CAN_BE_GCED(val))
    to_ret.tag = (jz_tag*)(val.value.gc);
  else if (JZ_TVAL_IS_NULL(val))
    to_ret.tag = NULL;
  else {
    /* TODO: This memory isn't freed.
       to get it to be,
       we'll need OOPs. */
    to_ret.val = malloc(sizeof(jz_tvalue));
    *to_ret.val = val;
  }

  return to_ret;
}

jz_tvalue jz_cons_ptr_unwrap(JZ_STATE, jz_cons_ptr val) {
  jz_tvalue to_ret;

  if (val.tag == NULL)
    return JZ_NULL;

  assert(JZ_TAG_TYPE(*val.tag) != jz_t_enum);
  assert(JZ_TAG_TYPE(*val.tag) != jz_t_cons);

  if (!JZ_TAG_CAN_BE_GCED(*val.tag))
    return *val.val;

  JZ_TVAL_SET_TYPE(to_ret, JZ_TAG_TYPE(*val.tag));
  to_ret.value.gc = val.gc;

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
  to_ret->car.tag = va_arg(args, jz_tag*);

  for (i = 1; i < argc; i++) {
    jz_cons* next = jz_cons_empty(jz);

    end->cdr.node = next;
    end = next;
    end->car.tag = va_arg(args, jz_tag*);
  }

  va_end(args);

  end->cdr.tag = NULL;

  return to_ret;
}

jz_cons* jz_list_concat(JZ_STATE, jz_cons* list1, jz_cons* list2) {
  jz_cons* next = list1;

  if (list1 == NULL) return list2;

  while (next->cdr.node != NULL) next = NODE(CDR(next));
  next->cdr.node = list2;

  return list1;
}


jz_cons* jz_list_reverse(JZ_STATE, jz_cons* head) {
  jz_cons *prev = NULL, *curr = NULL, *next = NULL;

  if (head == NULL) return NULL;

  curr = head;
  next = NODE(CDR(curr));

  while (next != NULL) {
    curr->cdr.node = prev;

    prev = curr;
    curr = next;
    next = NODE(CDR(next));
  }

  curr->cdr.node = prev;

  return curr;
}


void jz_cons_print(JZ_STATE, jz_cons* root) {
  print_parse_list(jz, root, jz_true);
  putchar('\n');
}

static void print_parse_list(JZ_STATE, jz_cons* node, jz_bool start) {
  if (start)
    putchar('(');

  print_parse_ptr(jz, CAR(node));

  if (node->cdr.tag == NULL)
    putchar(')');
  else if (TYPE(CDR(node)) == jz_t_cons) {
    putchar(' ');
    print_parse_list(jz, NODE(CDR(node)), jz_false);
  } else {
    printf(" . ");
    print_parse_ptr(jz, CDR(node));
    putchar(')');
  }
}

static void print_parse_ptr(JZ_STATE, jz_cons_ptr ptr) {
  if (ptr.tag == NULL) {
    printf("null");
    return;
  }

  switch (JZ_TAG_TYPE(*ptr.tag)) {
  case jz_t_enum:
    printf("%s", jz_parse_names[ptr.leaf->val]);
    break;

  case jz_t_cons:
    print_parse_list(jz, ptr.node, jz_true);
    break;

  case jz_t_void:
    printf("<void>");
    break;

  default: {
    char* str = jz_str_to_chars(jz, jz_to_str(jz, jz_cons_ptr_unwrap(jz, ptr)));
    printf("%s", str);
    free(str);
  }
  }
}

