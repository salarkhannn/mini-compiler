# mc — A C-like language compiler with 5-pass static optimization

[![CI](https://github.com/salarkhannn/mini-compiler/actions/workflows/ci.yml/badge.svg)](https://github.com/salarkhannn/mini-compiler/actions/workflows/ci.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)]()
[![Flex](https://img.shields.io/badge/Flex-2.6-blue)]()
[![Bison](https://img.shields.io/badge/Bison-3.0-blue)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow)]()

`mc` compiles a statically-typed C-like language through a complete 8-stage pipeline: lexing, LALR(1) parsing, semantic analysis, three-address code (TAC) generation, 5 optimization passes, and LLVM IR emission.

**57 → 37 instructions** on the demo program (35% reduction) in **~0.34 ms** of optimizer time.

## Quick Start

```bash
# Dependencies
sudo apt-get install flex bison g++ clang

# Build
make

# Compile and run
./minicc tests/test_main.mc
cat output/test_main.tac      # unoptimized TAC
cat output/test_main.opt.tac  # optimized TAC
```

## Features

- **Lexer** — Flex-based tokenizer with line-accurate tracking
- **Parser** — Bison LALR(1) grammar with full precedence hierarchy
- **Semantic Analysis** — type checking, scope management, function resolution, implicit widening
- **TAC IR** — 9 instruction forms targeting a virtual register machine
- **5 Optimization Passes** — constant folding/propagation, common subexpression elimination (CSE), dead code elimination (DCE), unreachable block elimination, loop-invariant code motion (LICM)
- **LLVM IR Backend** — C → LLVM IR via Clang with -O3 comparison
- **Standalone Tools** — postfix/prefix/infix calculators, First/Follow/LL(1) table generator

## Language

A small sample (`tests/test_main.mc`):

```c
int g;
float pi;

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    int prev;
    prev = factorial(n - 1);
    return n * prev;
}

int main() {
    int fact;
    fact = factorial(5);
    print fact;
    return 0;
}
```

Types: `int`, `float`, `void` • Control: `if`/`else`, `while` • Operators: `+`, `-`, `*`, `/`, `^`, `==`, `!=`, `<`, `>`, `<=`, `>=` • Functions with parameters • `print` built-in

## Architecture

<!-- PIPELINE_DIAGRAM -->

The compiler pipeline consists of 8 modular phases:

| Phase | Input | Output | Implementation |
|-------|-------|--------|---------------|
| Lexer | `.mc` source | Token stream | `src/lexer.l` (Flex) |
| Parser | Token stream | AST | `src/parser.y` (Bison LALR(1)) |
| Semantic Analysis | AST | Annotated AST | `src/semantic.cpp` |
| TAC Generation | AST | Three-address code | `src/tac_gen.cpp` |
| Constant Folding/Prop | TAC | Optimized TAC | `src/optimizer.cpp` |
| CSE | TAC | Optimized TAC | `src/optimizer.cpp` |
| Dead Code Elimination | TAC | Optimized TAC | `src/optimizer.cpp` |
| Unreachable Block Elim | TAC | Optimized TAC | `src/optimizer.cpp` |
| LICM | TAC | Optimized TAC | `src/optimizer.cpp` |

## Optimization Passes

All five passes run sequentially in a single pipeline:

### 1. Constant Folding + Constant Propagation
Evaluates compile-time-known expressions and propagates the results forward.
```
t6 = 3 + 4      →    result = 7.0
result = t6
t7 = result * 2 →    result = 14.0
result = t7
```

### 2. Common Subexpression Elimination (CSE)
Recomputes identical expressions within a basic block by reusing the earlier result.

### 3. Dead Code Elimination (DCE)
Removes synthetic temporaries that are written but never read (backwards liveness scan).

### 4. Unreachable Block Elimination
Removes code after unconditional jumps and labels not reachable from any branch.

### 5. Loop-Invariant Code Motion (LICM)
Hoists loop-invariant computations out of loop bodies to before the loop header.

### Benchmark

Measured on `tests/test_main.mc` (55 lines, 4 functions, recursion, loop, branch):

| Metric | Before | After | Reduction |
|--------|--------|-------|-----------|
| Total instructions | 57 | 37 | **35%** |
| Non-label instructions | 27 | 16 | **41%** |
| Optimizer runtime (avg, 5 runs) | — | 0.34 ms | — |

```
for i in 1 2 3 4 5; do ./minicc tests/test_main.mc --bench --no-ast --no-tokens 2>&1 | grep bench; done
[bench] optimiser: 0.334 ms
[bench] optimiser: 0.346 ms
[bench] optimiser: 0.351 ms
[bench] optimiser: 0.257 ms
[bench] optimiser: 0.419 ms
```

## Usage

```
./minicc <source-file> [options]

Options:
  --no-tokens    Suppress token dump
  --no-ast       Suppress AST dump
  --no-tac       Suppress unoptimized TAC dump
  --no-opt       Suppress optimized TAC dump
  --bench        Print optimizer wall-clock time
  --stats        Print instruction count before/after optimization
  --version      Print version and exit
  --help         Print this message and exit

Output files (written to output/<stem>.*):
  output/<stem>.tokens    Token stream
  output/<stem>.ast       Abstract syntax tree
  output/<stem>.tac       Unoptimized three-address code
  output/<stem>.opt.tac   Optimized three-address code
```

### Standalone Tools

```bash
make demos
echo "3 4 + 2 *"       | ./parser/postfix     # result = 14
echo "3 + 4 * 2"        | ./parser/calc_infix  # result = 11
echo "2^10"             | ./parser/calc_infix   # result = 1024
echo "log(2.718)"       | ./parser/calc_infix   # result = ~0.9999

make ff
./first_follow/ff         # FIRST/FOLLOW/LL(1) table
```

### LLVM IR

```bash
make llvm-ir
diff llvm/test1.ll llvm/test1_opt.ll  # compare -O3 effects
```

## Build

## Test Suite

```
$ make check
==== mc test suite ====

--- Compilation tests ---
  PASS  tests/fib.mc
  PASS  tests/optimizer_fold.mc
  PASS  tests/optimizer_licm.mc

--- Error detection tests ---
  PASS  type_error.mc
  PASS  scope_error.mc
  PASS  void_error.mc

--- Examples ---
  PASS  examples/fib.mc
  PASS  examples/collatz.mc
  PASS  examples/pi.mc

--- CLI flags ---
  PASS  --version
  PASS  --help
  PASS  --stats

==== Results: 12 passed, 0 failed ====
```

## Build

```
make              Build compiler binary (minicc) with debug symbols
make release      Build with -O2 for production use
make demos        Build postfix/prefix/infix calculators
make ff           Build First/Follow/LL(1) tool
make llvm-ir      Generate LLVM IR via Clang
make test         Compile and run on tests/test_main.mc
make check        Run full test suite (12 tests)
make clean        Remove all build artifacts
```

## Project Structure

```
├── src/              Core pipeline (lexer, parser, semantic, TAC, optimizer)
├── include/          Shared headers (AST, TAC, symbol table, error handler)
├── tests/            Sample .mc programs
├── examples/         Polished example programs
├── parser/           Standalone postfix/prefix/infix calculators
├── first_follow/     FIRST/FOLLOW/LL(1) table generator
├── llvm/             LLVM IR test programs and generated .ll files
├── docs/             Mermaid diagram sources
└── Makefile
```

## License

MIT
