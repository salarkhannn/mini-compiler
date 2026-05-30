// Recursive fibonacci — demonstrates function calls, recursion, branching
int fib(int n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int i;
    int val;
    i = 0;
    while (i <= 15) {
        val = fib(i);
        print val;
        i = i + 1;
    }
    return 0;
}
