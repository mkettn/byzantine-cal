#pragma once
#include <chrono>

namespace bcal {

// Date of Orthodox Pascha (Easter) in the Gregorian calendar.
// Valid for years 1583..9999 (throws std::out_of_range otherwise).
std::chrono::year_month_day orthodox_pascha(int year);

}  // namespace bcal
