#!/bin/bash
MC=./minicc
PASS=0
FAIL=0

green() { echo "  PASS  $1"; PASS=$((PASS + 1)); }
red()   { echo "  FAIL  $1"; FAIL=$((FAIL + 1)); }

echo "==== mc test suite ===="
echo ""
echo "--- Compilation tests ---"

for t in tests/fib tests/optimizer_fold tests/optimizer_licm; do
  $MC "${t}.mc" --no-ast --no-tokens 2>/dev/null && green "$t.mc" || red "$t.mc"
done

echo ""
echo "--- Error detection tests ---"

out=$($MC tests/type_error.mc --no-ast --no-tokens 2>&1)
echo "$out" | grep -q "narrowing" && green "type_error.mc" || red "type_error.mc"

out=$($MC tests/scope_error.mc --no-ast --no-tokens 2>&1)
echo "$out" | grep -q "undeclared" && green "scope_error.mc" || red "scope_error.mc"

out=$($MC tests/void_error.mc --no-ast --no-tokens 2>&1)
echo "$out" | grep -q "void" && green "void_error.mc" || red "void_error.mc"

echo ""
echo "--- Examples ---"

for t in examples/fib examples/collatz examples/pi; do
  $MC "${t}.mc" --no-ast --no-tokens 2>/dev/null && green "$t.mc" || red "$t.mc"
done

echo ""
echo "--- CLI flags ---"

$MC --version 2>/dev/null | grep -q "mc version" && green "--version" || red "--version"
$MC --help 2>/dev/null | grep -q "Usage:" && green "--help" || red "--help"
$MC tests/fib.mc --stats --no-ast --no-tokens 2>/dev/null | grep -q "reduction" && green "--stats" || red "--stats"

echo ""
echo "==== Results: $PASS passed, $FAIL failed ===="
exit $FAIL
