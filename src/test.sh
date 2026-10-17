#!/usr/bin/env bash
set -uo pipefail
export LC_ALL=C

script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd) || exit 1
compiler=${CC:-cc}
for tool in "$compiler" timeout awk cmp mktemp; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    printf 'FAIL: required tool not found: %s\n' "$tool" >&2
    exit 1
  fi
done
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/game24-test.XXXXXX") || exit 1
trap 'rm -rf -- "$build_dir"' EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if ! "$compiler" -std=c99 -Wall -Wextra -Wpedantic -O2 \
    "$script_dir/game24.c" -o "$build_dir/game24" -lm ||
   ! "$compiler" -std=c99 -Wall -Wextra -Wpedantic -O2 \
    "$script_dir/testgame.c" -o "$build_dir/testgame" -lm ||
   ! "$compiler" -std=c99 -Wall -Wextra -Wpedantic -O2 \
    "$script_dir/tests/reference.c" -o "$build_dir/reference"; then
  printf 'Summary: FAIL (compilation failed).\n' >&2
  exit 1
fi

if ! timeout 10s "$build_dir/reference" "$script_dir/tests"; then
  printf 'Summary: FAIL (independent fixture verification failed or timed out).\n' >&2
  exit 1
fi
mapfile -t doable < "$script_dir/tests/doable_in.txt"
mapfile -t expected < "$script_dir/tests/doable_out.txt"
mapfile -t impossible < "$script_dir/tests/impossible_in.txt"
printf 'No solution' > "$build_dir/impossible.expected"

passed=0
failed=0

show_file() {
  if [[ -s $2 ]]; then
    printf '  %s:\n' "$1"
    sed 's/^/    /' "$2"
    printf '\n'
  fi
}

run_solver() {
  if timeout 2s "$build_dir/game24" <<< "$1" \
      > "$build_dir/actual" 2> "$build_dir/solver.err"; then
    solver_status=0
  else
    solver_status=$?
  fi
  # Strip whitespace at the boundaries only, preserving internal content.
  awk '{ text = text $0 "\n" }
       END { sub(/^[[:space:]]*/, "", text);
             sub(/[[:space:]]*$/, "", text); printf "%s", text }' \
    "$build_dir/actual" > "$build_dir/trimmed" || return 1
}

for i in "${!doable[@]}"; do
  input=${doable[i]}
  read -r -a numbers <<< "$input"
  if timeout 2s "$build_dir/testgame" "${numbers[@]}" "${expected[i]}" \
      > "$build_dir/expected.canonical" 2> "$build_dir/expected.err"; then
    expected_status=0
  else
    expected_status=$?
  fi
  if ! run_solver "$input"; then
    printf 'Summary: FAIL (could not read solver output).\n' >&2
    exit 1
  fi
  if timeout 2s "$build_dir/testgame" "${numbers[@]}" \
      < "$build_dir/actual" > "$build_dir/actual.canonical" \
      2> "$build_dir/checker.err"; then
    actual_status=0
  else
    actual_status=$?
  fi
  if ((solver_status == 0 && expected_status == 0 && actual_status == 0)) &&
      cmp -s "$build_dir/expected.canonical" "$build_dir/actual.canonical"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    printf 'FAIL solvable #%d: %s\n' "$((i + 1))" "$input"
    printf '  exit statuses: solver=%d, actual checker=%d, expected checker=%d\n' \
      "$solver_status" "$actual_status" "$expected_status"
    printf '  expected expression: %s\n' "${expected[i]}"
    show_file 'actual output' "$build_dir/actual"
    show_file 'solver stderr' "$build_dir/solver.err"
    show_file 'actual checker' "$build_dir/checker.err"
    show_file 'expected checker' "$build_dir/expected.err"
  fi
done

for i in "${!impossible[@]}"; do
  input=${impossible[i]}
  if ! run_solver "$input"; then
    printf 'Summary: FAIL (could not read solver output).\n' >&2
    exit 1
  fi
  if ((solver_status == 0)) &&
      cmp -s "$build_dir/impossible.expected" "$build_dir/trimmed"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    printf 'FAIL impossible #%d: %s\n' "$((i + 1))" "$input"
    printf '  expected: No solution; solver exit status: %d\n' "$solver_status"
    show_file 'actual output' "$build_dir/actual"
    show_file 'solver stderr' "$build_dir/solver.err"
  fi
done

printf 'Summary: %d passed, %d failed (%d solvable, %d impossible).\n' \
  "$passed" "$failed" "${#doable[@]}" "${#impossible[@]}"
((failed == 0))
