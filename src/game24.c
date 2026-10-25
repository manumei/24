#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define EPSILON 1e-9
#define TEXT_SIZE 64 // much more than enough

typedef struct {
  double value;
  char text[TEXT_SIZE]; // operations to get to the number
} Number;

bool solve(Number numbers[], int count) {
  // only one number left, check whether we reached 24
  if (count == 1) {
    if (fabs(numbers[0].value - 24.0) < EPSILON) {
      printf("%s\n", numbers[0].text);
      return true;
    }
    return false;
  }

  // go through the remaining cards, try combinations
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < count; j++) {
      if (i == j) { // not same entry twice
        continue;
      }

      Number next[4];
      int remaining = 0;

      // copy all numbers except current pair
      for (int k = 0; k < count; k++) {
        if (k != i && k != j) {
          next[remaining++] = numbers[k];
        }
      }

      double a = numbers[i].value;
      double b = numbers[j].value;
      char operators[] = "+-*/";

      // try all operations on the pair
      for (int op = 0; op < 4; op++) {
        // commutative! a+b=b+a and a*b=b*a, so skip reversed duplicates
        if (i > j && (operators[op] == '+' || operators[op] == '*')) {
          continue;
        }
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
          if (fabs(b) < EPSILON) { // no division by zero
            continue;
          }
          result = a / b;
          break;
        default:
          continue;
        }

        // add the result after the remaining numbers
        next[remaining].value = result;

        // keep track of how the result was obtained
        int length = snprintf(next[remaining].text, TEXT_SIZE, "(%s %c %s)",
                              numbers[i].text, operators[op], numbers[j].text);

        // check for format errors or text that didn't fit (it should)
        if (length < 0 || length >= TEXT_SIZE) {
          continue;
        }

        // recurse (one fewer card)
        if (solve(next, count - 1)) {
          return true; // solution found, stop searching
        }
        // otherwise, continue
      }
    }
  }

  return false; // all combinations tried, no solution
}

int main(void) {
  Number numbers[4];

  // read and check the four input cards
  for (int i = 0; i < 4; i++) {
    int value;

    if (scanf("%d", &value) != 1 || value < 1 || value > 10) {
      printf("Invalid input.\n");
      return 1;
    }

    // at first each Number's text is just the number
    numbers[i].value = value;
    snprintf(numbers[i].text, TEXT_SIZE, "%d", value);
  }

  // solve prints the expression if a solution is found
  bool solution = solve(numbers, 4);

  if (!solution) {
    printf("No solution\n");
  }

  return 0;
}