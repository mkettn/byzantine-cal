# Agent Guidelines for byzantine-cal

C++20 CLI for Orthodox fasting calendars. It parses and validates the dates and date ranges of a calendar YAML file and generates the calendar of a year from it.

- `src/day.*` - date syntax parser (`DD.MM`, `p[+-]N[d|w][<|>wd]`)
- `src/validator.*` - checks the YAML structure, ranges, weekdays, rules
- `src/pascha.*` - Orthodox Pascha (Gregorian date) for a year
- `src/year_calendar.*` - `YearCalendar(year, offset_days)`: all days of a year; the constructor calculates Pascha; `add_day()` / `add_range()` attach a data string (later calls win; `offset_days` shifts fixed dates, leaping over New Year; no YAML in this class)
- `src/calendar_loader.*` - `load_year_calendar()`: calendar YAML -> `YearCalendar` (data = rule as one-line YAML)
- `src/main.cpp` - docopt CLI, spdlog output
- `cmake/Find*.cmake` - one module per dependency (find_package, else FetchContent)
- `tests/` - `test_main.cpp` (unit); `valid/*.yml` must pass, `invalid/*.yml` need an exact exit code and message (see CMakeLists.txt, `run_expect.cmake`)

Dates are `DD.MM` (the old `MMDD` form is rejected).
Keep new dependencies in their own `cmake/Find<lib>.cmake`.
