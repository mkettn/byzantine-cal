#include "year_calendar.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "day.h"
#include "pascha.h"
#include "validator.h"

namespace bcal {

namespace {

using namespace std::chrono;

// Weekday numbering of Day / weekday keys: 0 = mon .. 6 = sun.
int weekday_index(sys_days d) {
  return static_cast<int>((weekday{d}.c_encoding() + 6) % 7);
}

std::string to_yaml_line(const YAML::Node& n) {
  YAML::Emitter out;
  out.SetMapFormat(YAML::Flow);
  out.SetSeqFormat(YAML::Flow);
  out << n;
  return out.c_str();
}

// Resolves a Day for the given base year. Fixed dates are shifted by
// `offset`; Pascha-relative ones are not. Returns nullopt for dates that do
// not exist in that year (29.02. in a non-leap year).
std::optional<sys_days> resolve(const Day& d, int base_year, int offset,
                                int year_for_fixed) {
  if (d.kind == Day::Kind::Solar) {
    year_month_day ymd{std::chrono::year{year_for_fixed},
                       std::chrono::month(static_cast<unsigned>(d.month)),
                       std::chrono::day(static_cast<unsigned>(d.day))};
    if (!ymd.ok()) return std::nullopt;
    return sys_days{ymd} + days{offset};
  }
  if (base_year < kMinYear || base_year > kMaxYear) return std::nullopt;
  sys_days r = sys_days{orthodox_pascha(base_year)} + days{d.offset_days};
  if (d.next_weekday) {
    while (weekday_index(r) != *d.next_weekday) r += days{1};
  } else if (d.prev_weekday) {
    while (weekday_index(r) != *d.prev_weekday) r -= days{1};
  }
  return r;
}

}  // namespace

YearCalendar::YearCalendar(const YAML::Node& root, int year, int offset_days)
    : year_(year), offset_days_(offset_days) {
  if (year < kMinYear || year > kMaxYear)
    throw std::invalid_argument("year must be in 1..9999");
  if (offset_days < -365 || offset_days > 365)
    throw std::invalid_argument("offset must be within +-365 days");

  auto report = validate(root);
  if (!report.ok()) {
    const auto& e = report.errors.front();
    throw std::invalid_argument(
        (e.line > 0 ? "line " + std::to_string(e.line) + ": " : "") + e.message);
  }

  // All days of the year.
  const sys_days first{std::chrono::year{year} / January / 1};
  const sys_days last{std::chrono::year{year} / December / 31};
  for (sys_days d = first; d <= last; d += days{1})
    days_.push_back({year_month_day{d}, std::nullopt});

  auto apply = [&](sys_days d, const YAML::Node& value) {
    if (d < first || d > last) return;
    days_[static_cast<std::size_t>((d - first).count())].data =
        to_yaml_line(value);
  };

  // Entries are applied in order, so later ones win. Dates are resolved for
  // the previous, the requested and the next year: shifted fixed dates (and
  // long Pascha-relative ones) of a neighbouring year can land in this year.
  for (const auto& item : root) {
    for (const auto& kv : item) {
      const std::string key = kv.first.Scalar();
      const auto pos = key.find("..");

      // Fixed dates (offset <= 365 days) need the neighbouring years only.
      // Pascha-relative dates with a long offset can come from years further
      // away, so widen the range by the size of their offset.
      int span = 1;
      for (const auto& part :
           pos == std::string::npos
               ? std::vector<std::string>{key}
               : std::vector<std::string>{key.substr(0, pos), key.substr(pos + 2)}) {
        auto d = *parse_day(part).day;
        if (d.kind == Day::Kind::Pascha)
          span = std::max(span, std::abs(d.offset_days) / 365 + 2);
      }

      for (int base = year - span; base <= year + span; ++base) {
        if (pos == std::string::npos) {
          auto day = parse_day(key).day;
          auto date = resolve(*day, base, offset_days, base);
          if (date) apply(*date, kv.second);
          continue;
        }

        auto start = *parse_day(key.substr(0, pos)).day;
        auto end = *parse_day(key.substr(pos + 2)).day;
        // A fixed range whose end is "before" its start crosses New Year.
        int end_year = base;
        if (start.kind == Day::Kind::Solar && end.kind == Day::Kind::Solar &&
            (end.month < start.month ||
             (end.month == start.month && end.day < start.day)))
          end_year = base + 1;

        auto s = resolve(start, base, offset_days, base);
        auto e = resolve(end, base, offset_days, end_year);
        if (!s || !e) continue;

        // Weekday key -> data; later keys win.
        std::vector<std::pair<std::set<int>, YAML::Node>> rules;
        for (const auto& wk : kv.second) {
          std::string err;
          rules.emplace_back(parse_weekday_key(wk.first.Scalar(), err),
                             wk.second);
        }
        for (sys_days d = *s; d <= *e; d += days{1}) {
          if (d < first || d > last) continue;
          const YAML::Node* hit = nullptr;
          for (const auto& [wds, value] : rules)
            if (wds.count(weekday_index(d))) hit = &value;
          if (hit) apply(d, *hit);
        }
      }
    }
  }
}

const CalendarDay& YearCalendar::at(year_month_day date) const {
  if (!date.ok() || date.year() != std::chrono::year{year_})
    throw std::out_of_range("date is not in the calendar's year");
  const sys_days first{std::chrono::year{year_} / January / 1};
  return days_[static_cast<std::size_t>((sys_days{date} - first).count())];
}

const CalendarDay& YearCalendar::at(unsigned month, unsigned day) const {
  // Reject values that the chrono types would silently truncate.
  if (month > 12 || day > 31)
    throw std::out_of_range("not a valid month or day");
  return at(std::chrono::year{year_} / std::chrono::month{month} /
            std::chrono::day{day});
}

}  // namespace bcal
