# Agent Guidelines for byzantine-cal

C++20 CLI for Orthodox fasting calendars. It parses and validates the dates and date ranges of a calendar YAML file and generates the calendar of a year from it.

- `src/day.*` - date syntax parser (`DD.MM`, `p[+-]N[d|w][<|>wd]`)
- `src/validator.*` - checks the YAML structure, ranges, weekdays, rules
- `src/pascha.*` - Orthodox Pascha (Gregorian date) for a year
- `src/year_calendar.*` - `YearCalendar(year, offset_days)`: all days of a year; the constructor calculates Pascha; `add_day()` / `add_range()` attach a data string (later calls win; `offset_days` shifts fixed dates, leaping over New Year; no YAML in this class)
- `src/calendar_loader.*` - `load_year_calendar()`: calendar YAML -> `YearCalendar` (data = rule as one-line YAML)
- `src/template_backend.*` - `calendar_to_json()` (YearCalendar -> nlohmann JSON) and `render_template()` (inja), `TemplateBackend` (reads the template file whose path it is given) and `MarkdownBackend` (template `markdown.inja`); every output format is a template
- `src/byzantine_cal_config.*` - `ByzantineCalConfig`: reads the config file (`-C`, else `./bcal.yaml`, else the installed `config.yaml`); `template_dir` lists the template directories (absolute or relative to the config file); `find_template(name)` returns the path of the template file (throws if there is none), which goes straight into a backend; unknown options are an error; also `lang_dir` (language file directories) and `language`; `find_language_file(name)` returns `<name>.yaml` from the first `lang_dir` that has it
- `src/language.*` - `Language`: month/weekday names and `ui` texts from a language file (the value of `language:` / `--lang` is the file name without `.yaml`, not checked; no environment lookup); its `fallback` language is read first, keys of the file replace those of the fallback, every file is read once with a warning for a circular reference
- `config/config.yaml`, `tpl/*.inja`, `lang/*.yaml` - installed to `<prefix>/lib/byzantine-cal` (prefix = `CMAKE_INSTALL_PREFIX`, compiled in as `BCAL_CONFIG_FILE`); the program finds its templates through the config
- `src/main.cpp` - docopt CLI, spdlog output
- `cmake/Find*.cmake` - one module per dependency (find_package, else FetchContent)
- `tests/` - `test_main.cpp` (unit); `valid/*.yml` must pass, `invalid/*.yml` need an exact exit code and message (see CMakeLists.txt, `run_expect.cmake`)

Dates are `DD.MM` (the old `MMDD` form is rejected).
Keep new dependencies in their own `cmake/Find<lib>.cmake`.
