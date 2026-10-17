#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define EPSILON 1e-9
#define TEXT_SIZE 128

typedef struct {
  double value;
  char text[TEXT_SIZE];
} Number;

bool solve(Number numbers[], int count) {
  if (count == 1) {
    if (fabs(numbers[0].value - 24.0) < EPSILON) {
      printf("%s = 24\n", numbers[0].text);
      return true;
    }
    return false;
  }

  for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {
      if (i == j) {
        continue;
      }

      Number next[4];
      int remaining = 0;

      /* Copy all numbers except the chosen pair. */
      for (int k = 0; k < count; k++) {
        if (k != i && k != j) {
          next[remaining++] = numbers[k];
        }
      }

      double a = numbers[i].value;
      double b = numbers[j].value;
      char operators[] = "+-*/";

      for (int op = 0; op < 4; op++) {
        double result;

        switch (operators[op]) {
        case '+':
          result = a + b;
          break;
        case '-':
          result = a - b;
          break;
        case '*':
          result = a * b;
          break;
        case '/':
          if (fabs(b) < EPSILON) {
            continue;
          }
          result = a / b;
          break;
        default:
          continue;
        }

        /* Add the result after the remaining numbers. */
        next[remaining].value = result;
        int length = snprintf(next[remaining].text, TEXT_SIZE, "(%s %c %s)",
                              numbers[i].text, operators[op], numbers[j].text);

        if (length < 0 || length >= TEXT_SIZE) {
          continue;
        }

        if (solve(next, count - 1)) {
          return true;
        }
      }
    }
  }

  return false;
}

int main(void) {
  Number numbers[4];

  for (int i = 0; i < 4; i++) {
    int value;

    if (scanf("%d", &value) != 1 || value < 1 || value > 10) {
      printf("Invalid input.\n");
      return 1;
    }

    numbers[i].value = value;
    snprintf(numbers[i].text, TEXT_SIZE, "%d", value);
  }

  if (!solve(numbers, 4)) {
    printf("No solution.\n");
  }

  return 0;
}