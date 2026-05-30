// Approximate pi using Leibniz series: pi/4 = 1 - 1/3 + 1/5 - 1/7 + ...
// Demonstrates: float arithmetic, while loops, alternating series
int main() {
    float pi;
    float term;
    int n;
    int sign;
    pi   = 0.0;
    n    = 1;
    sign = 1;
    while (n < 100000) {
        term = 1.0 / n;
        if (sign == 1) {
            pi = pi + term;
        } else {
            pi = pi - term;
        }
        n    = n + 2;
        sign = 0 - sign;
    }
    print pi;
    return 0;
}
