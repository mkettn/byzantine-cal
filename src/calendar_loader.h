#pragma once
#include <yaml-cpp/yaml.h>

#include "year_calendar.h"

namespace bcal {

YearCalendar load_year_calendar(const YAML::Node& root, int year,
                                int offset_days = 0);

}  // namespace bcal
