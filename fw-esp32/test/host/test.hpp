// Tiny dependency-free test harness.
#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace t {

struct Case {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> r;
    return r;
}

inline int& failures() {
    static int f = 0;
    return f;
}

struct Register {
    Register(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

}  // namespace t

#define T_CAT2(a, b) a##b
#define T_CAT(a, b) T_CAT2(a, b)
#define TEST(name)                                                   \
    static void name();                                              \
    static t::Register T_CAT(reg_, name)(#name, name);               \
    static void name()

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);            \
            ++t::failures();                                                         \
        }                                                                            \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        const auto va_ = (a);                                                              \
        const auto vb_ = (b);                                                              \
        if (!(va_ == vb_)) {                                                               \
            std::printf("  FAIL %s:%d: %s == %s (%lld vs %lld)\n", __FILE__, __LINE__, #a, \
                        #b, (long long)va_, (long long)vb_);                               \
            ++t::failures();                                                               \
        }                                                                                  \
    } while (0)
