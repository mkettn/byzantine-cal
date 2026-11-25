#pragma once
#include <optional>
#include <string>

namespace bcal {

// A day specification. Syntax:
//   "DD.MM"                      fixed solar date, e.g. "25.12", "6.1"
//   "e[+-]N[d|w][<|>weekday]"    relative to Orthodox Easter, e.g. "e", "e+39",
//                                "e-7w", "e-7w<wed" (previous Wednesday)
struct Day {
  enum class Kind { Solar, Easter };
  Kind kind = Kind::Solar;
  int day = 0;    // Solar only
  int month = 0;  // Solar only
  int offset_days = 0;                 // Easter only
  std::optional<int> prev_weekday;     // '<', 0 = mon .. 6 = sun
  std::optional<int> next_weekday;     // '>'
};

struct DayResult {
  std::optional<Day> day;
  std::string error;  // set when !day
};

DayResult parse_day(const std::string& text);

// Weekday name ("mon".."sun", case-insensitive) -> 0..6.
std::optional<int> parse_weekday(const std::string& name);

}  // namespace bcal
