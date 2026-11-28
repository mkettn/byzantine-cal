#pragma once
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "day.h"

namespace bcal {

struct CalendarDay {
  std::chrono::year_month_day date;
  std::optional<std::string> data;

  bool has_rule() const { return data.has_value(); }
  std::chrono::weekday weekday() const {
    return std::chrono::weekday{std::chrono::sys_days{date}};
  }
};

class YearCalendar {
 public:
  using const_iterator = std::vector<CalendarDay>::const_iterator;

  explicit YearCalendar(int year, int offset_days = 0);

  int year() const { return year_; }
  int offset_days() const { return offset_days_; }
  std::size_t size() const { return days_.size(); }

  std::chrono::year_month_day pascha() const { return pascha_; }

  void add_day(const Day& day, std::string data);

  void add_range(const Day& start, const Day& end, const Weekdays& weekdays,
                 std::string data);

  const CalendarDay& at(std::chrono::year_month_day date) const;
  const CalendarDay& at(unsigned month, unsigned day) const;

  const_iterator begin() const { return days_.begin(); }
  const_iterator end() const { return days_.end(); }

 private:
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
