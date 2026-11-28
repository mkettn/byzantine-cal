# Dates and date ranges

A calendar file lists dates and date ranges, each with a rule or some data. This page describes
how dates are written, the format of the calendar file, and how a year is generated from it.

## Dates

Dates are written **day first, then month, separated by a dot**. A date is either fixed or
relative to Orthodox Pascha.

| Syntax | Meaning |
|---|---|
| `25.12`, `6.1`, `06.01` | A fixed date. The leading zero is optional. `29.02` is accepted. |
| `p` | Pascha. |
| `p+39`, `p+39d`, `p-7d` | Days after or before Pascha. `d` is the default unit. |
| `p-7w`, `p+1w` | Weeks after or before Pascha. |
| `p-7w<wed` | The result moved back to the previous Wednesday (or kept, if it is one). |
| `p+1w>sun` | The result moved forward to the next Sunday (or kept, if it is one). |

Weekdays are `mon`, `tue`, `wed`, `thu`, `fri`, `sat` and `sun`.

## Date ranges

A range is written `start..end`. Both ends are included.

| Syntax | Meaning |
|---|---|
| `01.02..14.02` | A range of fixed dates. |
| `15.12..05.01` | An end before the start means the range crosses New Year. |
| `p-49d..p` | A range between Pascha-relative dates. The end must not be before the start. |
| `01.01..p` | A mix of both. It is resolved within the year, and a range that ends before it starts is empty. |

## The calendar file

A calendar file is a YAML list with a single document. Every item is `date: value`:

```yaml
- "06.01": Theophany                 # a name
- "25.03":
    name: Annunciation
    rule: no_fish
- "p+39": Ascension
- "01.02..14.02":                    # a range: rules for weekdays
    mon..fri: no_meat
    sat,sun: no_dairy
- "25.12":                           # other fields are arbitrary data
    name: Nativity
    rule: no_fish
    icon: nativity.svg
- "15.11..24.12":
    mon..fri: {rule: no_meat, note: "strict"}   # a rule with arbitrary data
```

**Always put dates in quotes** (`"06.01"`). Some YAML tools read an unquoted `06.01` as the number
6.01.

- **A fixed date or a Pascha-relative date** has a value that is either a name (plain text) or a
  map with a required `name`, an optional `rule` and any further fields.
- **A range** has a map as its value. Its keys are weekdays and each value is a *rule*: either a
  rule name (`no_meat`) or a map with a required `rule` and any further fields.
- **Rule names** are `no_fast`, `no_meat`, `no_dairy`, `no_fish` and `no_oil`.
- **Weekday keys** are lowercase and are either one range of weekdays (`mon..fri`, which may wrap:
  `sat..mon`) or a comma list (`wed,fri`). A range needs at least one weekday key.
- **Later wins.** The same date, or overlapping weekdays in one range, may appear more than once.
  The later entry replaces the earlier one for a day.

`byzantine-cal calendar.yml` checks a file. Problems are reported with the line they are on, for
example an impossible date (`31.04`), an unknown rule or weekday, or a range without rules.

## Generating a year

```bash
byzantine-cal --year=2026 calendar.yml
byzantine-cal --year=2026 --old calendar.yml
```

`--year` generates the calendar of that year, 1 January to 31 December, and renders it with a
[template](templates.md). Each day gets the data of the rule that applies to it, as the YAML of the
rule on a single line, for example `no_meat` or `{name: Nativity, rule: no_fish}`.

- **Later rules win** where several apply to the same day, in the order of the file.
- **All dates in the output are Gregorian.** `--old` is for a calendar written in the old (Julian)
  calendar: it adds 13 days to every fixed date, so the rule for 25.12. is found on 7 January.
  The shift also crosses year boundaries: 25.12. of the previous year lands on 7 January of the
  requested year. Weekday rules use the weekday of the shifted date.
- **Pascha-relative dates are not shifted.** They refer to the Pascha of the requested year, and a
  date that falls outside the year is ignored.
- **29.02.** exists only in leap years. In other years the entry has no effect, and a range with
  29.02. as one of its ends is ignored as a whole.
- **Years** from 1 to 9999 are accepted. The 13-day difference between the calendars is exact from
  1900 to 2099, and used for every year. The offset is meant to be small and is not checked.

## Using it from C++

`YearCalendar` (`src/year_calendar.h`) is the calendar of one year. It is created for a year and an
offset, which is added to the fixed dates. The constructor calculates the day of Pascha. It is
filled with rules that carry a string of arbitrary data, and the rule added last wins for a day:

```cpp
bcal::YearCalendar cal(2026, /*offset_days=*/13);   // cal.pascha() is 12 April 2026

auto day = [](const char* s) { return *bcal::parse_day(s).day; };
cal.add_day(day("25.12"), "Nativity");
cal.add_range(day("15.11"), day("24.12"), {0, 1, 2, 3, 4}, "no_meat");          // mon..fri
cal.add_range(day("p"), day("p+6d"), bcal::all_weekdays(), "{name: Bright Week}");

const bcal::CalendarDay& d = cal.at(1, 7);   // 7 January (also takes a std::chrono::year_month_day)
if (d.has_rule()) std::cout << *d.data;
for (const bcal::CalendarDay& each : cal) { /* 365 or 366 days, in order */ }
```

All dates outside the class are Gregorian: `at()`, the returned days and the iteration. With an
offset of 13, `at(12, 25)` returns the rule that was added for 12.12. The class knows nothing about
YAML.

- **Weekdays** are numbers, 0 (Monday) to 6 (Sunday), in a `bcal::Weekdays` set. `all_weekdays()`
  has all of them.
- **`add_day()` and `add_range()`** ignore dates that do not exist or fall outside the year. The ends
  of a range are included, a range of fixed dates whose end is before its start crosses New Year,
  and a range with a bound that does not exist is ignored as a whole.
- **Days** are `CalendarDay`s: the `date`, the `weekday()`, `has_rule()` and the `data` string of the
  rule, which is empty without one. `size()` is 365 or 366, and `pascha()` is the Gregorian date of
  Pascha, not shifted by the offset.
- **Errors:** the constructor throws `std::invalid_argument` for a year outside 1..9999, and `at()`
  throws `std::out_of_range` for a date that is not in the year.

`parse_day()` (`src/day.h`) turns the text of a date into a `Day`, or returns an error message.
`validate()` (`src/validator.h`) checks a parsed calendar file and returns the problems with their
lines, and `load_year_calendar()` (`src/calendar_loader.h`) builds a calendar from a calendar file.
It throws `std::invalid_argument` if the file is invalid or the year is out of range, and the data of
a day is the YAML of its rule on one line.
