#pragma once
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace bcal {

// One day of a generated calendar.
struct CalendarDay {
  std::chrono::year_month_day date;
  // Arbitrary data attached by the rule that applies to this day, as a
  // single-line YAML string (e.g. "no_meat" or "{name: Nativity, rule: no_fish}").
  // Empty if no rule applies.
  std::optional<std::string> data;

  bool has_rule() const { return data.has_value(); }
  std::chrono::weekday weekday() const {
    return std::chrono::weekday{std::chrono::sys_days{date}};
  }
};

// The calendar of one year (1 January .. 31 December), generated from a
// calendar document.
//
// Rules are applied in document order; where several rules apply to the same
// day, the later one wins.
//
// `offset_days` is added to every fixed (DD.MM) date, so that e.g. an offset
// of 13 moves 25.12 to 7 January. Dates are shifted across year boundaries:
// 25.12. of the previous year lands on 7 January of the requested year.
// Pascha-relative dates are not shifted. Offset must be within +-365 days.
class YearCalendar {
 public:
  using const_iterator = std::vector<CalendarDay>::const_iterator;

  // `root` must be a valid calendar document (see validate()); throws
  // std::invalid_argument if it is not, or if year (1900..2099) / offset
  // are out of range.
  YearCalendar(const YAML::Node& root, int year, int offset_days = 0);

  int year() const { return year_; }
  int offset_days() const { return offset_days_; }
  std::size_t size() const { return days_.size(); }  // 365 or 366

  // Query a day. Throws std::out_of_range if it is not in this year.
  const CalendarDay& at(std::chrono::year_month_day date) const;
  const CalendarDay& at(unsigned month, unsigned day) const;

  // Iterate over all days of the year, in order.
  const_iterator begin() const { return days_.begin(); }
  const_iterator end() const { return days_.end(); }

 private:
  int year_;
  int offset_days_;
  std::vector<CalendarDay> days_;
};

}  // namespace bcal
