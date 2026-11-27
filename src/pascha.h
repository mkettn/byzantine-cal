#pragma once
#include <chrono>

namespace bcal {

// Difference between the Gregorian and the Julian calendar in days. It is
// exactly 13 from 1 March 1900 to 28 February 2100 and used for all years;
// outside that range results may be off by some days.
inline constexpr int kJulianGregorianOffsetDays = 13;

// Years that are accepted (anything the arithmetic can handle).
inline constexpr int kMinYear = 1;
inline constexpr int kMaxYear = 9999;

// Date of Orthodox Pascha (Easter) in the Gregorian calendar.
// Throws std::out_of_range for years outside kMinYear..kMaxYear.
std::chrono::year_month_day orthodox_pascha(int year);

}  // namespace bcal
