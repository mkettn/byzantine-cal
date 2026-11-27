#include "pascha.h"

#include <stdexcept>

namespace bcal {

std::chrono::year_month_day orthodox_pascha(int year) {
  using namespace std::chrono;
  if (year < kMinYear || year > kMaxYear)
    throw std::out_of_range("year must be in 1..9999");

  // Meeus' Julian algorithm gives the date in the Julian calendar.
  int a = year % 4, b = year % 7, c = year % 19;
  int d = (19 * c + 15) % 30;
  int e = (2 * a + 4 * b - d + 34) % 7;
  int month = (d + e + 114) / 31;
  int day = (d + e + 114) % 31 + 1;

  // Julian -> Gregorian: add the (constant) difference of the two calendars.
  sys_days julian_as_gregorian{
      year_month_day{std::chrono::year{year}, std::chrono::month(static_cast<unsigned>(month)),
                     std::chrono::day(static_cast<unsigned>(day))}};
  return year_month_day{julian_as_gregorian + days{kJulianGregorianOffsetDays}};
}

}  // namespace bcal
