#pragma once
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "year_calendar.h"

namespace bcal {

// Throws std::invalid_argument, with the line, for anything wrong in the calendar.
YearCalendar load_year_calendar(const YAML::Node& root, int year, int offset_days = 0,
                                const std::filesystem::path& file = {});

// Reads the file first: ConfigFileError if it cannot be read, ConfigError for invalid YAML.
YearCalendar load_year_calendar_file(const std::filesystem::path& file, int year,
                                     int offset_days = 0);

}  // namespace bcal
