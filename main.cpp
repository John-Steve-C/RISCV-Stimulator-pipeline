#include "src/cpu.hpp"

#include <bits/stdc++.h>

using namespace std;

CPU cpu;

int main() {
//    freopen("testcases_for_riscv/testcases/qsort.data", "r", stdin);
//    freopen("output.txt", "w", stdout);

    cpu.input();
    cpu.run();

    fclose(stdin), fclose(stdout);

    return 0;
}