#pragma once
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "day.h"

namespace bcal {

// One day of a calendar.
struct CalendarDay {
  std::chrono::year_month_day date;
  // Arbitrary data attached by the rule that applies to this day. Empty if no
  // rule applies.
  std::optional<std::string> data;

  bool has_rule() const { return data.has_value(); }
  std::chrono::weekday weekday() const {
    return std::chrono::weekday{std::chrono::sys_days{date}};
  }
};

// The calendar of one year (1 January .. 31 December), created for a year and
// a fixed offset and filled with rules by add_day() / add_range(). Each rule
// carries a string of arbitrary data; the rule added last wins for a day.
//
// All dates outside of this class are Gregorian (at(), CalendarDay::date,
// iteration). The fixed dates (DD.MM) of added rules are shifted by
// `offset_days`, also across year boundaries: with 13, 25.12. of the previous
// year is 7 January and at(12, 25) returns the rule added for 12.12.
// Pascha relative dates refer to the Pascha of this year and are not shifted.
class YearCalendar {
 public:
  using const_iterator = std::vector<CalendarDay>::const_iterator;

  // Calculates the day of Pascha. Throws std::invalid_argument if year is not
  // in 1..9999. The offset is meant to be small (13 for the old calendar) and
  // is not checked.
  explicit YearCalendar(int year, int offset_days = 0);

  int year() const { return year_; }
  int offset_days() const { return offset_days_; }
  std::size_t size() const { return days_.size(); }  // 365 or 366

  // Pascha of this year (Gregorian date, not shifted by the offset).
  std::chrono::year_month_day pascha() const { return pascha_; }

  // Attaches data to a single day. Dates that do not exist (29.02. in a
  // non-leap year) or fall outside this year are ignored.
  void add_day(const Day& day, std::string data);

  // Attaches data to the days of a range (both ends included) that fall on one
  // of the given weekdays (0 = mon .. 6 = sun). A range of fixed dates whose
  // end is before its start crosses New Year. A range with a bound that does
  // not exist (29.02. in a non-leap year) is ignored as a whole.
  void add_range(const Day& start, const Day& end, const Weekdays& weekdays,
                 std::string data);

  // Query a day. Throws std::out_of_range if it is not in this year.
  const CalendarDay& at(std::chrono::year_month_day date) const;
  const CalendarDay& at(unsigned month, unsigned day) const;

  // Iterate over all days of the year, in order.
  const_iterator begin() const { return days_.begin(); }
  const_iterator end() const { return days_.end(); }

 private:
  // Fixed dates are taken in `solar_year`; nullopt if the date does not exist.
  std::optional<std::chrono::sys_days> resolve(const Day& d,
                                               int solar_year) const;
  void apply(std::chrono::sys_days first, std::chrono::sys_days last,
             const Weekdays& weekdays, const std::string& data);

  int year_;
  int offset_days_;
  std::chrono::year_month_day pascha_;
  std::vector<CalendarDay> days_;
};

}  // namespace bcal
