#pragma once
#include <optional>
#include <set>
#include <string>

namespace bcal {

// A day specification. Syntax:
//   "DD.MM"                      fixed solar date, e.g. "25.12", "6.1"
//   "p[+-]N[d|w][<|>weekday]"    relative to Orthodox Pascha, e.g. "p", "p+39",
//                                "p-7w", "p-7w<wed" (previous Wednesday)
struct Day {
  enum class Kind { Solar, Pascha };
  Kind kind = Kind::Solar;
  int day = 0;    // Solar only
  int month = 0;  // Solar only
  int offset_days = 0;                 // Pascha only
  std::optional<int> prev_weekday;     // '<', 0 = mon .. 6 = sun
  std::optional<int> next_weekday;     // '>'
};

struct DayResult {
  std::optional<Day> day;
  std::string error;  // set when !day
};

DayResult parse_day(const std::string& text);

// A set of weekdays, 0 = mon .. 6 = sun.
using Weekdays = std::set<int>;
inline Weekdays all_weekdays() { return {0, 1, 2, 3, 4, 5, 6}; }

// Weekday name ("mon".."sun", case-insensitive) -> 0..6.
std::optional<int> parse_weekday(const std::string& name);

// Parses a weekday key of a range entry: either one range ("mon..fri", wrapping
// allowed, e.g. "sat..mon") or a comma list ("wed,fri"). Lowercase only.
// Returns the covered weekdays (0 = mon .. 6 = sun); on failure the set is
// empty and `err` is set.
Weekdays parse_weekday_key(const std::string& key, std::string& err);

}  // namespace bcal
