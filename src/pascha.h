#pragma once
#include <chrono>

namespace bcal {

// Difference between the Gregorian and the Julian calendar in days. It is
// 13 from 1 March 1900 to 28 February 2100, which is the range supported here.
inline constexpr int kJulianGregorianOffsetDays = 13;

// Years for which orthodox_pascha() (and the year calendar) is supported.
inline constexpr int kMinYear = 1900;
inline constexpr int kMaxYear = 2099;

// Date of Orthodox Pascha (Easter) in the Gregorian calendar.
// Throws std::out_of_range for years outside kMinYear..kMaxYear.
std::chrono::year_month_day orthodox_pascha(int year);

}  // namespace bcal
