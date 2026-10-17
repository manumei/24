/* Independent fixture verifier: exact fractions and exhaustive subset DP.
 * Build: cc -std=c99 -Wall -Wextra -Wpedantic reference.c -o reference
 * Run:   ./reference /path/to/src/tests
 */
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int64_t n, d;
} Rational;

typedef struct {
  Rational *values;
  size_t count, capacity;
} Set;

static void die(const char *message) {
  fprintf(stderr, "Reference FAIL: %s\n", message);
  exit(1);
}

static Rational rational(int64_t n, int64_t d) {
  if (d == 0) {
    die("division by zero");
  }
  if (d < 0) {
    n = -n;
    d = -d;
  }
  int64_t a = n < 0 ? -n : n, b = d;
  while (b) {
    int64_t remainder = a % b;
    a = b;
    b = remainder;
  }
  return (Rational){n / a, d / a};
}

/* With at most four leaves in 1..10, numerator and denominator magnitudes
 * are bounded by 2^(4-1) * 10^4 = 80000, well within int64_t. */
static Rational calculate(Rational a, Rational b, char op) {
  switch (op) {
  case '+': return rational(a.n * b.d + b.n * a.d, a.d * b.d);
  case '-': return rational(a.n * b.d - b.n * a.d, a.d * b.d);
  case '*': return rational(a.n * b.n, a.d * b.d);
  case '/': return rational(a.n * b.d, a.d * b.n);
  default: die("unknown operator");
  }
  return (Rational){0, 1};
}

static void insert(Set *set, Rational value) {
  for (size_t i = 0; i < set->count; i++) {
    if (set->values[i].n == value.n && set->values[i].d == value.d) {
      return;
    }
  }
  if (set->count == set->capacity) {
    size_t capacity = set->capacity ? 2 * set->capacity : 32;
    Rational *values = realloc(set->values, capacity * sizeof(*values));
    if (!values) {
      die("out of memory");
    }
    set->values = values;
    set->capacity = capacity;
  }
  set->values[set->count++] = value;
}

static int solvable(const int inputs[4]) {
  Set sets[16] = {{0}};
  int found = 0;
  for (unsigned i = 0; i < 4; i++) {
    insert(&sets[1u << i], rational(inputs[i], 1));
  }
  /* Each binary tree splits into two disjoint nonempty leaf subsets.
   * Try every such partition, both subtraction/division orders, and every
   * exact result from its children. Equal-valued results can be merged. */
  for (unsigned mask = 1; mask < 16 && !found; mask++) {
    if ((mask & (mask - 1)) == 0) {
      continue;
    }
    for (unsigned left = (mask - 1) & mask; left && !found;
         left = (left - 1) & mask) {
      unsigned right = mask ^ left;
      if (left >= right) {
        continue;
      }
      for (size_t i = 0; i < sets[left].count && !found; i++) {
        for (size_t j = 0; j < sets[right].count && !found; j++) {
          Rational a = sets[left].values[i], b = sets[right].values[j];
          Rational candidates[6] = {
              calculate(a, b, '+'), calculate(a, b, '-'),
              calculate(b, a, '-'), calculate(a, b, '*')};
          size_t count = 4;
          if (b.n != 0) candidates[count++] = calculate(a, b, '/');
          if (a.n != 0) candidates[count++] = calculate(b, a, '/');
          for (size_t k = 0; k < count; k++) {
            if (mask == 15) {
              if (candidates[k].n == 24 && candidates[k].d == 1) {
                found = 1;
              }
            } else {
              insert(&sets[mask], candidates[k]);
            }
          }
        }
      }
    }
  }
  for (unsigned mask = 1; mask < 16; mask++) {
    free(sets[mask].values);
  }
  return found;
}

static int precedence(char op) {
  return (op == '*' || op == '/') ? 2 : (op == '+' || op == '-') ? 1 : 0;
}

static int reduce(Rational values[], size_t *count, char op) {
  if (*count < 2) return 0;
  Rational b = values[--*count], a = values[--*count];
  if (op == '/' && b.n == 0) return 0;
  values[(*count)++] = calculate(a, b, op);
  return 1;
}

/* A stack evaluator, separate from testgame's recursive descent parser. */
static int valid_expression(const char *text, const int inputs[4]) {
  Rational values[4];
  char operators[128];
  size_t nv = 0, no = 0, leaves = 0;
  int used[4] = {0}, need_operand = 1;
  while (*text) {
    if (isspace((unsigned char)*text)) {
      text++;
      continue;
    }
    if (isdigit((unsigned char)*text) && need_operand) {
      errno = 0;
      char *end;
      long n = strtol(text, &end, 10);
      if (errno == ERANGE || n < 1 || n > 10 || nv == 4) return 0;
      int i;
      for (i = 0; i < 4; i++) {
        if (!used[i] && n == inputs[i]) {
          used[i] = 1;
          break;
        }
      }
      if (i == 4) return 0;
      values[nv++] = rational(n, 1);
      leaves++;
      text = end;
      need_operand = 0;
    } else if (*text == '(' && need_operand) {
      if (no == sizeof operators) return 0;
      operators[no++] = *text++;
    } else if (*text == ')' && !need_operand) {
      while (no && operators[no - 1] != '(') {
        if (!reduce(values, &nv, operators[--no])) return 0;
      }
      if (!no) return 0;
      no--;
      text++;
    } else if (precedence(*text) && !need_operand) {
      while (no && precedence(operators[no - 1]) >= precedence(*text)) {
        if (!reduce(values, &nv, operators[--no])) return 0;
      }
      if (no == sizeof operators) return 0;
      operators[no++] = *text++;
      need_operand = 1;
    } else {
      return 0;
    }
  }
  if (need_operand) return 0;
  while (no) {
    char op = operators[--no];
    if (op == '(' || !reduce(values, &nv, op)) return 0;
  }
  return leaves == 4 && nv == 1 && values[0].n == 24 && values[0].d == 1;
}

static FILE *open_fixture(const char *directory, const char *name) {
  char path[4096];
  int length = snprintf(path, sizeof path, "%s/%s", directory, name);
  if (length < 0 || (size_t)length >= sizeof path) die("fixture path too long");
  FILE *file = fopen(path, "r");
  if (!file) {
    perror(path);
    exit(1);
  }
  return file;
}

static int line(FILE *file, char *text, size_t capacity) {
  if (!fgets(text, (int)capacity, file)) {
    if (ferror(file)) die("fixture read error");
    return 0;
  }
  if (!strchr(text, '\n') && !feof(file)) die("fixture line too long");
  return 1;
}

static void parse_inputs(const char *text, int inputs[4]) {
  for (int i = 0; i < 4; i++) {
    while (isspace((unsigned char)*text)) text++;
    if (!isdigit((unsigned char)*text)) die("expected four integers in 1..10");
    errno = 0;
    char *end;
    long n = strtol(text, &end, 10);
    if (errno == ERANGE || n < 1 || n > 10) die("input outside 1..10");
    if (*end && !isspace((unsigned char)*end)) die("inputs need whitespace");
    inputs[i] = (int)n;
    text = end;
  }
  while (isspace((unsigned char)*text)) text++;
  if (*text) die("trailing input content");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s fixture-directory\n", argv[0]);
    return 2;
  }
  const char *names[] = {"doable_in.txt", "impossible_in.txt"};
  for (int group = 0; group < 2; group++) {
    FILE *in = open_fixture(argv[1], names[group]);
    FILE *out = group == 0 ? open_fixture(argv[1], "doable_out.txt") : NULL;
    char input[4096], expected[4096];
    size_t count = 0;
    while (line(in, input, sizeof input)) {
      count++;
      int numbers[4];
      parse_inputs(input, numbers);
      if (solvable(numbers) != (group == 0)) {
        fprintf(stderr, "Reference FAIL: wrong classification in %s:%zu: %s",
                names[group], count, input);
        return 1;
      }
      if (out && (!line(out, expected, sizeof expected) ||
                  !valid_expression(expected, numbers))) {
        fprintf(stderr, "Reference FAIL: invalid exact expression at case %zu\n",
                count);
        return 1;
      }
    }
    if (count != (group == 0 ? 40u : 20u)) die("incorrect fixture line count");
    if (out && line(out, expected, sizeof expected)) die("extra expected expression");
    fclose(in);
    if (out) fclose(out);
  }
  puts("Reference PASS: 40 exact expressions; 20 exhaustively proven impossible cases.");
  return 0;
}
