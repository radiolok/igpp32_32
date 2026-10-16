#include "test.hpp"

int main() {
    int failed_cases = 0;
    for (const auto& c : t::registry()) {
        const int before = t::failures();
        c.fn();
        const bool ok = t::failures() == before;
        std::printf("%s %s\n", ok ? "[ OK ]" : "[FAIL]", c.name);
        failed_cases += ok ? 0 : 1;
    }
    std::printf("\n%zu tests, %d failed\n", t::registry().size(), failed_cases);
    return failed_cases ? 1 : 0;
}
