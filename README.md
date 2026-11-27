# byzantine-cal

Orthodox fasting calendar tool. It reads a calendar YAML file and checks that
all dates, date ranges and rules are valid.

## Date syntax

Dates are written **day first, then month, separated by a dot**:

| Syntax | Meaning |
|---|---|
| `25.12` / `6.1` / `06.01` | Fixed date (leading zero optional). `29.02` is accepted (leap years). |
| `01.02..14.02` | Range of fixed dates. A range may cross New Year (`15.12..05.01`); the validator accepts it. |
| `p`, `p+39`, `p-7d`, `p-7w` | Relative to Orthodox Pascha (days, or weeks with `w`). |
| `p-7w<wed` / `p+1w>sun` | Then move to the previous (`<`) / next (`>`) weekday. |
| `p-49d..p` | Range between Pascha-relative dates (the end must not be before the start). |

## File format

```yaml
- "06.01": Theophany                 # date: name
- "25.03":
    name: Annunciation
    rule: no_fish
- "p+39": Ascension
- "01.02..14.02":                    # range: weekday rules
    mon..fri: no_meat
    sat,sun: no_dairy
- "25.12":                           # extra fields are arbitrary data
    name: Nativity
    rule: no_fish
    icon: nativity.svg
- "15.11..24.12":
    mon..fri: {rule: no_meat, note: "strict"}   # rule + arbitrary data
```

**Always put dates in quotes** (`"06.01"`); some YAML tools read an unquoted `06.01` as the number 6.01.
A date may appear more than once; later entries override earlier ones.
A file must contain a single YAML document.

A rule is either a rule name (`no_meat`) or a map with a required `rule` and any further fields.
Fixed entries need a `name`, `rule` is optional, and any other field is allowed.

Rules: `no_fast`, `no_meat`, `no_dairy`, `no_fish`, `no_oil`.
Weekdays (lowercase): `mon tue wed thu fri sat sun`, written either as one range (`mon..fri`, `sat..mon`) or a comma list (`wed,fri`). If weekdays overlap within one range entry (`mon..fri` then `wed`), later keys override earlier ones.

## Build

Requires CMake >= 3.24 and a C++20 compiler. Dependencies
(yaml-cpp, spdlog, docopt.cpp) are resolved by `cmake/Find<lib>.cmake`: an
installed copy is used if found, otherwise it is fetched with FetchContent.

Installed libraries of these versions **or newer** are accepted (`find_package` minimum
version). If none is found, exactly these versions, the ones shipped in Ubuntu 24.04 LTS
(`libyaml-cpp-dev`, `libspdlog-dev`, `libdocopt-dev`), are fetched:

| Library | Minimum / fetched version |
|---|---|
| yaml-cpp | 0.8.0 |
| spdlog | 1.12.0 |
| docopt.cpp | 0.6.3 |

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

## Year calendar

`YearCalendar` (`src/year_calendar.h`) is the calendar of one year, from 1 January to
31 December. It is created for a year and a fixed offset; the constructor calculates the
day of Pascha. It is then filled with rules, each carrying a string of arbitrary data:

```cpp
bcal::YearCalendar cal(2026, /*offset_days=*/13);   // cal.pascha() is 12 April 2026

auto day = [](const char* s) { return *bcal::parse_day(s).day; };
cal.add_day(day("25.12"), "Nativity");
cal.add_day(day("p+39"), "{name: Ascension}");
cal.add_range(day("15.11"), day("24.12"), {0, 1, 2, 3, 4}, "no_meat");   // mon..fri
cal.add_range(day("p"), day("p+6d"), "no_fast");                           // all weekdays

const bcal::CalendarDay& d = cal.at(1, 7);   // or cal.at(2026y / January / 7)
if (d.has_rule()) std::cout << *d.data;

for (const bcal::CalendarDay& day : cal) { /* 365 or 366 days, in order */ }
```

- Where several rules apply to a day, the one added last wins.
- All dates outside the class are Gregorian: the dates passed to `at()`, the dates of the
  returned days and the dates printed by the command line. Only the fixed dates (`DD.MM`) of
  the added rules are shifted by `offset_days`, so `at(12, 25)` returns different data
  depending on the offset: the rule added for 25.12. with 0, the rule added for 12.12. with 13.
- The shift leaps over year boundaries: with 13, 25.12. of the previous year falls on
  7 January. Weekday filters use the weekday of the shifted date.
- Pascha-relative dates refer to the Pascha of the calendar's year and are not shifted.
- 29.02. only exists in leap years; in other years the rule has no effect. A range with
  29.02. as one of its bounds is ignored as a whole in such years.
- Years 1..9999 are accepted. The 13-day difference between the calendars is exact from 1900 to 2099.
- `offset_days` is meant to be small (13 for the old calendar). It is not checked, and large offsets are not handled.

`load_year_calendar()` (`src/calendar_loader.h`) builds the calendar of a year from a
calendar file (a `YAML::Node`). The data of a day is then the YAML of its rule as a single
line, e.g. `no_meat` or `{name: Nativity, rule: no_fish}`.

## Usage

```bash
byzantine-cal [(-v | -q)] [--year=<year> [--old]] <yaml_file>
```

Without `--year` the file is only checked. With `--year` the calendar of that year is
generated and every day that has a rule is printed as `YYYY-MM-DD<TAB>data`:

```bash
byzantine-cal --year=2026 calendar.yml
byzantine-cal --year=2026 --old calendar.yml   # old calendar: 25.12. -> 7 January
```

Exit status: `0` valid, `1` invalid (errors are logged to stderr with line numbers), `2` file unreadable (or a directory), `3` wrong command line.
