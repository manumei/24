Run `../test.sh` from this directory, or invoke `src/test.sh` using its relative
or absolute path from another working directory.
It compiles into a temporary directory and removes that directory on exit.
Each solver and checker invocation has a two-second timeout (`timeout` is
required). Set `CC` to choose a C compiler; the default is `cc`.

`reference.c` independently verifies all fixtures on every test run. Its
exhaustive subset search stores reduced fractions with integer numerators and
denominators, considers every binary tree partition and both orders for `-`
and `/`, and merges equal results. It uses neither game24 nor floating point.
A separate stack evaluator checks the expected expressions exactly, including
operand multiplicities. The 40 solvable fixtures include different expression
shapes, duplicates, fractional intermediates, and both range boundaries. The
20 impossible fixtures include `6 7 7 8`. All input numbers are in 1-10;
the reference verifier enforces this range.

For standalone use, compile `testgame.c` with C99 and `-lm`, then run:

```sh
./testgame 3 3 8 8 '8 / (3 - 8 / 3)'
printf '%s\n' '(8 / (3 - (8 / 3))) = 24' | ./testgame 3 3 8 8
```

The checker accepts either one expression argument or stdin, with at most
4096 bytes and 128 levels of parentheses. Operands are decimal C `int` values;
signs on integer literals are supported. Each operand must match an unused
original input. Arithmetic uses normal operator precedence and left
associativity. The only output wrapper accepted is the optional `= 24`
suffix. The current solver has no prompt; unexpected text is rejected. Success
prints only `24` and failure prints a diagnostic to stderr and exits nonzero.

Impossible cases must print `No solution`, ignoring surrounding whitespace
only.

`../timer.sh` compiles once into a temporary directory and runs `timer.py`.
The timer uses seed 24 and 1000 inputs in 1-10, captures stdout/stderr, and
checks process exit status with a two-second timeout. Timing includes process
startup and output capture, and excludes compilation. It checks execution
failures; expression correctness is checked by `test.sh`.
