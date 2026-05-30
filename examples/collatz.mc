// Collatz conjecture step counter — demonstrates while loops, if/else, arithmetic
int collatz(int n) {
    int steps;
    steps = 0;
    while (n != 1) {
        if (n / 2 * 2 == n) {
            n = n / 2;
        } else {
            n = 3 * n + 1;
        }
        steps = steps + 1;
    }
    return steps;
}

int main() {
    int n;
    int s;
    n = 1;
    while (n <= 20) {
        s = collatz(n);
        print n;
        print s;
        n = n + 1;
    }
    return 0;
}
