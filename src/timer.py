#!/usr/bin/env python3
"""Time 1,000 reproducible solver processes, including process startup."""

import argparse
import random
import subprocess
import time
from pathlib import Path

RUNS = 1000
SEED = 24
TIMEOUT = 2.0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "executable", nargs="?", type=Path,
        default=Path(__file__).resolve().with_name("game24"),
    )
    executable = parser.parse_args().executable.resolve()
    rng = random.Random(SEED)
    inputs = [" ".join(str(rng.randint(1, 10)) for _ in range(4))
              for _ in range(RUNS)]
    completed = failures = attempted = 0
    print(f"Timing {RUNS} inputs in 1-10; seed={SEED}; timeout={TIMEOUT:g}s.")
    print("Timing includes process startup and output capture; excludes compilation.")
    start = time.perf_counter()
    for index, numbers in enumerate(inputs, 1):
        attempted += 1
        try:
            result = subprocess.run(
                [str(executable)], input=numbers + "\n", text=True,
                capture_output=True, errors="replace", timeout=TIMEOUT, check=False,
            )
        except subprocess.TimeoutExpired as error:
            failures += 1
            print(f"FAIL run {index}, input [{numbers}]: timeout after {TIMEOUT:g}s")
            if error.stdout or error.stderr:
                print(f"  stdout={error.stdout!r}; stderr={error.stderr!r}")
            continue
        except OSError as error:
            failures += 1
            print(f"FAIL run {index}, input [{numbers}]: {error}")
            break  # The executable cannot be launched; further attempts add no information.
        completed += 1
        if result.returncode != 0:
            failures += 1
            reason = (f"crashed with signal {-result.returncode}"
                      if result.returncode < 0 else f"exit status {result.returncode}")
            print(f"FAIL run {index}, input [{numbers}]: {reason}")
            print(f"  stdout={result.stdout!r}; stderr={result.stderr!r}")
    elapsed = time.perf_counter() - start
    print(f"Completed runs: {completed}/{RUNS}; attempted: {attempted}")
    print(f"Failures: {failures}")
    print(f"Total elapsed time: {elapsed:.6f} seconds")
    print(f"Average time per run: {elapsed / attempted:.6f} seconds (per attempt)")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
