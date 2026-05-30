/*
 * Arithmetic loop: sum of squares of integers 1..N.
 * Used to demonstrate LLVM IR generation and -O3 optimisation.
 */
#include <stdio.h>

int sumSquares(int n) {
    int sum = 0;
    int i   = 1;
    while (i <= n) {
        sum = sum + i * i;
        i   = i + 1;
    }
    return sum;
}

int main(void) {
    printf("sumSquares(10) = %d\n", sumSquares(10));
    printf("sumSquares(100) = %d\n", sumSquares(100));
    return 0;
}
