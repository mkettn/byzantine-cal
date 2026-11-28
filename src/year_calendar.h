#pragma once
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "day.h"
#include "saint.h"

namespace bcal {

// A saint celebrated on a day, and which of the feasts of the saint it is. Both belong to the
// YearCalendar.
struct SaintFeast {
  const Saint* saint;
  const Feast* feast;
};

struct CalendarDay {
  std::chrono::year_month_day date;
  std::optional<std::string> data;
  std::vector<SaintFeast> saints;  // in the order the saints were added

  bool has_rule() const { return data.has_value(); }
  std::chrono::weekday weekday() const {
    return std::chrono::weekday{std::chrono::sys_days{date}};
  }
};

class YearCalendar {
 public:
  using const_iterator = std::vector<CalendarDay>::const_iterator;

  explicit YearCalendar(int year, int offset_days = 0);

  // The days point to the saints, so a copy would point into the original.
  YearCalendar(const YearCalendar&) = delete;
  YearCalendar& operator=(const YearCalendar&) = delete;
  YearCalendar(YearCalendar&&) = default;
  YearCalendar& operator=(YearCalendar&&) = default;

  int year() const { return year_; }
  int offset_days() const { return offset_days_; }
  std::size_t size() const { return days_.size(); }

  std::chrono::year_month_day pascha() const { return pascha_; }

  void add_day(const Day& day, std::string data);

  void add_range(const Day& start, const Day& end, const Weekdays& weekdays,
                 std::string data);

  // The calendar keeps the saint, and its feasts are added to the days like the dates of add_day().
  const Saint& add_saint(Saint saint);

  const CalendarDay& at(std::chrono::year_month_day date) const;
  const CalendarDay& at(unsigned month, unsigned day) const;

  const_iterator begin() const { return days_.begin(); }
  const_iterator end() const { return days_.end(); }

 private:
  std::optional<std::chrono::sys_days> resolve(const Day& d,
                                               int solar_year) const;
  void apply(std::chrono::sys_days first, std::chrono::sys_days last,
             const Weekdays& weekdays, const std::function<void(CalendarDay&)>& set);
  void place(const Day& start, const Day& end, const Weekdays& weekdays,
             const std::function<void(CalendarDay&)>& set);

  int year_;
  int offset_days_;
  std::chrono::year_month_day pascha_;
  std::vector<CalendarDay> days_;
  std::deque<Saint> saints_;  // a deque: the days point to its elements
};

}  // namespace bcal
