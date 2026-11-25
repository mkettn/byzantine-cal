# Agent Guidelines for byzantine-cal

C++20 CLI for Orthodox fasting calendars. It parses and validates the dates and date ranges of a calendar YAML file.

- `src/day.*` - date syntax parser (`DD.MM`, `e[+-]N[d|w][<|>wd]`)
- `src/validator.*` - checks the YAML structure, ranges, weekdays, rules
- `src/main.cpp` - docopt CLI, spdlog output
- `cmake/Find*.cmake` - one module per dependency (find_package, else FetchContent)
- `tests/` - `test_main.cpp` (unit); `valid/*.yml` must pass, `invalid/*.yml` need an exact exit code and message (see CMakeLists.txt, `run_expect.cmake`)

Dates are `DD.MM` (the old `MMDD` form is rejected).
Keep new dependencies in their own `cmake/Find<lib>.cmake`.
