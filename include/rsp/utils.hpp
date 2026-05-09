#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace rsp {

constexpr double INF = 1e100;
constexpr double EPS = 1e-9;

inline bool is_inf(double x) {
    return x >= INF / 2.0;
}

inline double safe_add(double a, double b) {
    if (is_inf(a) || is_inf(b)) return INF;
    if (a > INF / 2.0 - b) return INF;
    return a + b;
}

inline double finite_abs_diff(double a, double b) {
    if (is_inf(a) && is_inf(b)) return 0.0;
    if (is_inf(a) || is_inf(b)) return INF;
    return std::abs(a - b);
}

inline bool less_with_eps(double a, double b) {
    return a < b - EPS;
}

inline std::string format_value(double x) {
    return is_inf(x) ? "inf" : std::to_string(x);
}

}  // namespace rsp

