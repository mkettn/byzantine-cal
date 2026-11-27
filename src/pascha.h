#pragma once
#include <chrono>

namespace bcal {

// Gregorian minus Julian calendar in days: exactly 13 from 1900 to 2099, and
// used for all years (results outside that range may be off).
inline constexpr int kJulianGregorianOffsetDays = 13;

inline constexpr int kMinYear = 1;
inline constexpr int kMaxYear = 9999;

// Date of Orthodox Pascha in the Gregorian calendar. Throws std::out_of_range
// for years outside kMinYear..kMaxYear.
std::chrono::year_month_day orthodox_pascha(int year);

}  // namespace bcal
