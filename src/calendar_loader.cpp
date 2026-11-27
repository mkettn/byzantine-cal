#include "calendar_loader.h"

#include <stdexcept>
#include <string>

#include "day.h"
#include "validator.h"

namespace bcal {

namespace {

std::string to_yaml_line(const YAML::Node& n) {
  YAML::Emitter out;
  out.SetMapFormat(YAML::Flow);
  out.SetSeqFormat(YAML::Flow);
  out << n;
  return out.c_str();
}

}  // namespace

YearCalendar load_year_calendar(const YAML::Node& root, int year,
                                int offset_days) {
  YearCalendar calendar(year, offset_days);

  auto report = validate(root);
  if (!report.ok()) {
    const auto& e = report.errors.front();
    throw std::invalid_argument(
        (e.line > 0 ? "line " + std::to_string(e.line) + ": " : "") + e.message);
  }

  for (const auto& item : root) {
    for (const auto& kv : item) {
      const std::string key = kv.first.Scalar();
      const auto pos = key.find("..");

      if (pos == std::string::npos) {
        calendar.add_day(*parse_day(key).day, to_yaml_line(kv.second));
        continue;
      }

      const Day start = *parse_day(key.substr(0, pos)).day;
      const Day end = *parse_day(key.substr(pos + 2)).day;
      // One rule per weekday key; later keys win.
      for (const auto& wk : kv.second) {
        std::string err;
        calendar.add_range(start, end, parse_weekday_key(wk.first.Scalar(), err),
                           to_yaml_line(wk.second));
      }
    }
  }
  return calendar;
}

}  // namespace bcal
