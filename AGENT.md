# Agent Guidelines for byzantine-cal

C++20 CLI that validates a fasting-calendar YAML file (v0.1: validation only).

- `src/day.*` - date syntax parser (`DD.MM`, `e[+-]N[d|w][<|>wd]`)
- `src/validator.*` - checks the YAML structure, ranges, weekdays, rules
- `src/main.cpp` - docopt CLI, spdlog output
- `cmake/Find*.cmake` - one module per dependency (find_package, else FetchContent)
- `tests/` - `test_main.cpp` (unit), `valid.yml`, `invalid.yml` (run via ctest)

Differences from the Python `byzantine` project: dates are `DD.MM`, not `MMDD`.
Keep new dependencies in their own `cmake/Find<lib>.cmake`.
