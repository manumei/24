#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT 4096
#define MAX_DEPTH 128

typedef struct {
  const char *cursor;
  const char *error;
  int inputs[4];
  int used[4];
  unsigned depth;
} Parser;

static void skip_space(Parser *p) {
  while (isspace((unsigned char)*p->cursor)) {
    p->cursor++;
  }
}

static double expression(Parser *p);

static double operand(Parser *p) {
  skip_space(p);
  if (p->error) {
    return 0;
  }
  if (*p->cursor == '(') {
    if (++p->depth > MAX_DEPTH) {
      p->error = "parentheses nested too deeply";
      return 0;
    }
    p->cursor++;
    double value = expression(p);
    skip_space(p);
    if (!p->error && *p->cursor != ')') {
      p->error = "missing closing parenthesis";
    }
    if (!p->error) {
      p->cursor++;
    }
    p->depth--;
    return value;
  }

  /* A sign may belong to an integer literal, never to a parenthesized term. */
  const char *digits = p->cursor;
  if (*digits == '+' || *digits == '-') {
    digits++;
  }
  if (!isdigit((unsigned char)*digits)) {
    p->error = "expected an integer or parenthesized expression";
    return 0;
  }
  errno = 0;
  char *end;
  long value = strtol(p->cursor, &end, 10);
  if (errno == ERANGE || value < INT_MIN || value > INT_MAX) {
    p->error = "integer operand out of range";
    return 0;
  }
  p->cursor = end;
  for (int i = 0; i < 4; i++) {
    if (!p->used[i] && p->inputs[i] == value) {
      p->used[i] = 1;
      return (double)value;
    }
  }
  p->error = "operand is not available in the original inputs";
  return 0;
}

static double product(Parser *p) {
  double value = operand(p);
  while (!p->error) {
    skip_space(p);
    char op = *p->cursor;
    if (op != '*' && op != '/') {
      break;
    }
    p->cursor++;
    double rhs = operand(p);
    if (p->error) {
      break;
    }
    if (op == '/' && rhs == 0.0) {
      p->error = "division by zero";
      break;
    }
    value = op == '*' ? value * rhs : value / rhs;
    if (!isfinite(value)) {
      p->error = "non-finite arithmetic result";
    }
  }
  return value;
}

static double expression(Parser *p) {
  double value = product(p);
  while (!p->error) {
    skip_space(p);
    char op = *p->cursor;
    if (op != '+' && op != '-') {
      break;
    }
    p->cursor++;
    double rhs = product(p);
    if (p->error) {
      break;
    }
    value = op == '+' ? value + rhs : value - rhs;
    if (!isfinite(value)) {
      p->error = "non-finite arithmetic result";
    }
  }
  return value;
}

int main(int argc, char **argv) {
  if (argc != 5 && argc != 6) {
    fprintf(stderr, "Usage: %s n1 n2 n3 n4 [expression]\n"
                    "Without an expression argument, read it from stdin.\n",
            argv[0]);
    return 2;
  }
  Parser p = {0};
  for (int i = 0; i < 4; i++) {
    errno = 0;
    char *end;
    long value = strtol(argv[i + 1], &end, 10);
    if (end == argv[i + 1] || *end || errno == ERANGE ||
        value < INT_MIN || value > INT_MAX) {
      fprintf(stderr, "FAIL: invalid input integer: %s\n", argv[i + 1]);
      return 1;
    }
    p.inputs[i] = (int)value;
  }

  char text[MAX_TEXT + 1];
  size_t length;
  if (argc == 6) {
    length = strlen(argv[5]);
    if (length > MAX_TEXT) {
      fprintf(stderr, "FAIL: expression too long\n");
      return 1;
    }
    memcpy(text, argv[5], length + 1);
  } else {
    length = fread(text, 1, MAX_TEXT, stdin);
    int extra = fgetc(stdin);
    if (ferror(stdin) || extra != EOF || memchr(text, '\0', length)) {
      fprintf(stderr, "FAIL: unreadable, binary, or oversized expression\n");
      return 1;
    }
    text[length] = '\0';
  }

  /* game24.c currently emits no prompt. Its optional suffix is exactly '= 24'. */
  p.cursor = text;
  double value = expression(&p);
  skip_space(&p);
  if (!p.error && *p.cursor == '=') {
    p.cursor++;
    skip_space(&p);
    if (strncmp(p.cursor, "24", 2) != 0) {
      p.error = "expected '= 24' suffix";
    } else {
      p.cursor += 2;
      skip_space(&p);
    }
  }
  if (!p.error && *p.cursor) {
    p.error = "unexpected trailing content";
  }
  for (int i = 0; i < 4 && !p.error; i++) {
    if (!p.used[i]) {
      p.error = "expression does not use all four input numbers";
    }
  }
  if (!p.error && (!isfinite(value) || fabs(value - 24.0) > 1e-9)) {
    p.error = "expression does not evaluate to 24";
  }
  if (p.error) {
    fprintf(stderr, "FAIL: %s\n", p.error);
    return 1;
  }
  puts("24");
  return 0;
}
