#pragma once
#include <yaml-cpp/yaml.h>

#include "year_calendar.h"

namespace bcal {

// Builds the calendar of `year` from a calendar document (see validate()).
// The data of every day is the YAML of its rule as a single line, e.g.
// "no_meat" or "{name: Nativity, rule: no_fish}". Rules are added in document
// order, so later ones win.
// Throws std::invalid_argument if the document is invalid or year / offset_days
// are out of range (see YearCalendar).
YearCalendar load_year_calendar(const YAML::Node& root, int year,
                                int offset_days = 0);

}  // namespace bcal
