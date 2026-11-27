# Agent Guidelines for byzantine-cal

C++20 CLI for Orthodox fasting calendars. It parses and validates the dates and date ranges of a calendar YAML file and generates the calendar of a year from it.

- `src/day.*` - date syntax parser (`DD.MM`, `p[+-]N[d|w][<|>wd]`)
- `src/validator.*` - checks the YAML structure, ranges, weekdays, rules
- `src/pascha.*` - Orthodox Pascha (Gregorian date) for a year
- `src/year_calendar.*` - `YearCalendar(year, offset_days)`: all days of a year; the constructor calculates Pascha; `add_day()` / `add_range()` attach a data string (later calls win; `offset_days` shifts fixed dates, leaping over New Year; no YAML in this class)
- `src/calendar_loader.*` - `load_year_calendar()`: calendar YAML -> `YearCalendar` (data = rule as one-line YAML)
- `src/template_backend.*` - `calendar_to_json()` (YearCalendar -> nlohmann JSON) and `render_template()` (inja); every output format is a template, no format code in C++
- `tpl/*.inja` - templates, installed to `<prefix>/lib/byzantine-cal/tpl` (prefix = `CMAKE_INSTALL_PREFIX`, compiled in as `BCAL_TPL_DIR`) and read from there at run time
- `src/main.cpp` - docopt CLI, spdlog output
- `cmake/Find*.cmake` - one module per dependency (find_package, else FetchContent)
- `tests/` - `test_main.cpp` (unit); `valid/*.yml` must pass, `invalid/*.yml` need an exact exit code and message (see CMakeLists.txt, `run_expect.cmake`)

Dates are `DD.MM` (the old `MMDD` form is rejected).
Keep new dependencies in their own `cmake/Find<lib>.cmake`.
