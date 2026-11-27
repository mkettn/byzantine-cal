#pragma once
#include <yaml-cpp/yaml.h>

#include "year_calendar.h"

namespace bcal {

// Builds the calendar of `year` from a calendar document. The data of a day is
// the YAML of its rule as a single line, e.g. "{name: Nativity, rule: no_fish}".
// Throws std::invalid_argument if the document is invalid or the year is out of
// range.
YearCalendar load_year_calendar(const YAML::Node& root, int year,
                                int offset_days = 0);

}  // namespace bcal
