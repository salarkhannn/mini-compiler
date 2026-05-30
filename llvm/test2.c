/*
 * Fibonacci and Collatz: branching, recursion, and loops.
 * Used to demonstrate LLVM IR generation and -O3 optimisation.
 */
#include <stdio.h>

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int collatz(int n) {
    int steps = 0;
    while (n != 1) {
        if (n % 2 == 0)
            n = n / 2;
        else
            n = 3 * n + 1;
        steps = steps + 1;
    }
    return steps;
}

int main(void) {
    printf("fib(10)       = %d\n", fib(10));
    printf("fib(20)       = %d\n", fib(20));
    printf("collatz(27)   = %d steps\n", collatz(27));
    return 0;
}
