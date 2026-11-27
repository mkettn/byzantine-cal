#include "year_calendar.h"

#include <algorithm>
#include <stdexcept>

#include "pascha.h"

namespace bcal {

namespace {

using namespace std::chrono;

// Weekday numbering of Day / weekday keys: 0 = mon .. 6 = sun.
int weekday_index(sys_days d) {
  return static_cast<int>((weekday{d}.c_encoding() + 6) % 7);
}

bool is_solar(const Day& d) { return d.kind == Day::Kind::Solar; }

}  // namespace

YearCalendar::YearCalendar(int year, int offset_days)
    : year_(year), offset_days_(offset_days) {
  if (year < kMinYear || year > kMaxYear)
    throw std::invalid_argument("year must be in 1..9999");

  pascha_ = orthodox_pascha(year);

  const sys_days first{std::chrono::year{year} / January / 1};
  const sys_days last{std::chrono::year{year} / December / 31};
  for (sys_days d = first; d <= last; d += days{1})
    days_.push_back({year_month_day{d}, std::nullopt});
}

std::optional<sys_days> YearCalendar::resolve(const Day& d,
                                              int solar_year) const {
  if (is_solar(d)) {
    year_month_day ymd{std::chrono::year{solar_year},
                       std::chrono::month(static_cast<unsigned>(d.month)),
                       std::chrono::day(static_cast<unsigned>(d.day))};
    if (!ymd.ok()) return std::nullopt;
    return sys_days{ymd} + days{offset_days_};
  }
  sys_days r = sys_days{pascha_} + days{d.offset_days};
  if (d.next_weekday) {
    while (weekday_index(r) != *d.next_weekday) r += days{1};
  } else if (d.prev_weekday) {
    while (weekday_index(r) != *d.prev_weekday) r -= days{1};
  }
  return r;
}

void YearCalendar::apply(sys_days from, sys_days to, const Weekdays& weekdays,
                         const std::string& data) {
  const sys_days first{std::chrono::year{year_} / January / 1};
  const sys_days last{std::chrono::year{year_} / December / 31};
  // Only the part inside this year matters.
  for (sys_days d = std::max(from, first); d <= std::min(to, last);
       d += days{1})
    if (weekdays.count(weekday_index(d)))
      days_[static_cast<std::size_t>((d - first).count())].data = data;
}

void YearCalendar::add_day(const Day& day, std::string data) {
  add_range(day, day, all_weekdays(), std::move(data));
}

void YearCalendar::add_range(const Day& start, const Day& end,
                             const Weekdays& weekdays, std::string data) {
  if (!is_solar(start) || !is_solar(end)) {
    // Pascha relative (or mixed): relative to this year.
    auto s = resolve(start, year_);
    auto e = resolve(end, year_);
    if (s && e) apply(*s, *e, weekdays, data);
    return;
  }
  // A range of fixed dates whose end is before its start crosses New Year.
  // Dates of the previous or the next year can be shifted into this year.
  const bool crosses =
      end.month < start.month || (end.month == start.month && end.day < start.day);
  for (int y = year_ - 1; y <= year_ + 1; ++y) {
    auto s = resolve(start, y);
    auto e = resolve(end, crosses ? y + 1 : y);
    if (s && e) apply(*s, *e, weekdays, data);
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
