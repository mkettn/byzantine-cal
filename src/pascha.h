#pragma once
#include <chrono>

namespace bcal {

// Difference between the Gregorian and the Julian calendar in days. It is
// 13 from 1 March 1900 to 28 February 2100, which is the range supported here.
inline constexpr int kJulianGregorianOffsetDays = 13;

// Years for which orthodox_pascha() is supported (the range of the constant).
inline constexpr int kPaschaMinYear = 1900;
inline constexpr int kPaschaMaxYear = 2099;

// Calendars are generated for these years.
inline constexpr int kMinYear = 2026;
inline constexpr int kMaxYear = kPaschaMaxYear;

// Date of Orthodox Pascha (Easter) in the Gregorian calendar.
// Throws std::out_of_range for years outside kPaschaMinYear..kPaschaMaxYear.
std::chrono::year_month_day orthodox_pascha(int year);

}  // namespace bcal
