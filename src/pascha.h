#pragma once
#include <chrono>

namespace bcal {

// Exact from 1900 to 2099, used for all years.
inline constexpr int kJulianGregorianOffsetDays = 13;

inline constexpr int kMinYear = 1;
inline constexpr int kMaxYear = 9999;

std::chrono::year_month_day orthodox_pascha(int year);

}  // namespace bcal
