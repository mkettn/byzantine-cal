#pragma once
#include <optional>
#include <set>
#include <string>

namespace bcal {

struct Day {
  enum class Kind { Solar, Pascha };
  Kind kind = Kind::Solar;
  int day = 0;                      // Solar only
  int month = 0;                    // Solar only
  int offset_days = 0;              // Pascha only
  std::optional<int> prev_weekday;  // '<', 0 = mon .. 6 = sun
  std::optional<int> next_weekday;  // '>'
};

struct DayResult {
  std::optional<Day> day;
  std::string error;
};

DayResult parse_day(const std::string& text);

// A set of weekdays, 0 = mon .. 6 = sun.
using Weekdays = std::set<int>;
inline Weekdays all_weekdays() { return {0, 1, 2, 3, 4, 5, 6}; }

std::optional<int> parse_weekday(const std::string& name);

// A range of Pascha-relative days that ends before it starts can never match (not checked with a
// weekday shift, which can move the ends). A range of fixed dates may cross New Year.
bool ends_before_start(const Day& start, const Day& end);

Weekdays parse_weekday_key(const std::string& key, std::string& err);

}  // namespace bcal
