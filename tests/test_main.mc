// Test input for the mini-compiler pipeline.
// Exercises: global vars, functions, if/else, while, recursion,
//            type promotion, exponentiation via call.

int g;
float pi;

int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    int prev;
    prev = factorial(n - 1);
    return n * prev;
}

void demo(int x, float y) {
    int result;
    float acc;
    result = 3 + 4;
    result = result * 2;
    acc    = y + 1.5;
    while (x > 0) {
        acc    = acc + y;
        result = result - 1;
        x      = x - 1;
    }
    print result;
    print acc;
}

int main() {
    int a;
    int b;
    int m;
    int fact;
    a    = 10;
    b    = 20;
    g    = 0;
    pi   = 3.14159;
    m    = max(a, b);
    fact = factorial(5);
    print m;
    print fact;
    demo(3, pi);
    return 0;
}
