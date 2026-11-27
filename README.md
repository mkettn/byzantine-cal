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
(yaml-cpp, spdlog, docopt.cpp, nlohmann_json, inja) are resolved by `cmake/Find<lib>.cmake`: an
installed copy is used if found, otherwise it is fetched with FetchContent.

Installed libraries of these versions **or newer** are accepted (`find_package` minimum
version). If none is found, exactly these versions are fetched. They are the ones shipped in
Ubuntu 24.04 LTS (`libyaml-cpp-dev`, `libspdlog-dev`, `libdocopt-dev`, `nlohmann-json3-dev`),
except for inja, which Ubuntu does not package:

| Library | Minimum / fetched version |
|---|---|
| yaml-cpp | 0.8.0 |
| spdlog | 1.12.0 |
| docopt.cpp | 0.6.3 |
| nlohmann_json | 3.11.3 |
| inja | 3.4.0 |

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

## Install

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local   # the default prefix
cmake --build build -j
cmake --install build
```

The program goes to `<prefix>/bin`, the configuration `config/config.yaml` and the templates in
`tpl/` to `<prefix>/lib/byzantine-cal` (`/usr/local/lib/byzantine-cal/config.yaml` by default).
The prefix is compiled in, so it has to be set when configuring, not when installing.

## Configuration

The configuration is read from a YAML file (`ByzantineCalConfig`, `src/byzantine_cal_config.h`):

```yaml
template_dir:          # directories to look for templates, in this order
  - /usr/share/tpl     # an absolute name, or
  - tpl                # a name relative to the directory of this file
```

Unknown options are an error. The file is, in this order:

1. the one given with `-C <file>`,
2. `bcal.yaml` in the current directory,
3. `config.yaml` in the installation directory (`/usr/local/lib/byzantine-cal/config.yaml`),
   which names the installed `tpl` directory.

## Year calendar

`YearCalendar` (`src/year_calendar.h`) is the calendar of one year, 1 January to 31 December.
It is created for a year and a fixed offset (the constructor calculates the day of Pascha) and
filled with rules, each carrying a string of arbitrary data:

```cpp
bcal::YearCalendar cal(2026, /*offset_days=*/13);   // cal.pascha() is 12 April 2026

auto day = [](const char* s) { return *bcal::parse_day(s).day; };
cal.add_day(day("25.12"), "Nativity");
cal.add_range(day("15.11"), day("24.12"), {0, 1, 2, 3, 4}, "no_meat");          // mon..fri
cal.add_range(day("p"), day("p+6d"), bcal::all_weekdays(), "{name: Bright Week}");

const bcal::CalendarDay& d = cal.at(1, 7);   // 7 January (also takes a std::chrono::year_month_day)
if (d.has_rule()) std::cout << *d.data;
for (const bcal::CalendarDay& d : cal) { /* 365 or 366 days, in order */ }
```

- The rule added last wins for a day.
- All dates outside the class are Gregorian (`at()`, the returned days, the command line output).
  Only the fixed `DD.MM` dates of added rules are shifted by `offset_days`, also across year
  boundaries: with 13, the rule added for 25.12. is found on 7 January of the following year,
  and `at(12, 25)` returns the rule added for 12.12. Weekday filters use the shifted date.
- Pascha-relative dates refer to the Pascha of the calendar's year and are not shifted.
- 29.02. only exists in leap years; in other years the rule has no effect, and a range with
  29.02. as a bound is ignored as a whole.
- Years 1..9999 are accepted. The 13-day difference between the calendars is exact from 1900
  to 2099. The offset is meant to be small (13) and is not checked.

`load_year_calendar()` (`src/calendar_loader.h`) builds the calendar from a calendar file
(a `YAML::Node`); the data of a day is the YAML of its rule as a single line.

## Template backends

A backend (`src/template_backend.h`) renders the `YearCalendar` with an
[inja](https://github.com/pantor/inja) template read from a file, so a new output format (HTML,
ICS, ...) is another template with its own backend class. `MarkdownBackend`'s template is
`markdown.inja` (`tpl/markdown.inja` in the sources, installed with the program, see
[Install](#install)); the program finds it in the `template_dir` of the [configuration](#configuration).

```cpp
bcal::MarkdownBackend backend("/usr/local/lib/byzantine-cal/tpl/markdown.inja");
std::string text = backend.render(calendar);   // TemplateFileError / TemplateError on failure
```

Templates get the calendar as JSON (`calendar_to_json()`): `year`, `offset_days`, `pascha`
(`YYYY-MM-DD`), all `days` and the `months` (`number`, `name`, `rule_count`, `days`). A day has
`date` (`YYYY-MM-DD`), `year`, `month`, `day`, `weekday` (1 = Monday .. 7 = Sunday),
`weekday_name`, `has_rule` and `data` (empty without a rule). Besides the inja built-ins there is
`escape_markdown(text)`, which backslash-escapes everything that has a meaning in Markdown
(newlines become `<br>`). Blocks (`{% ... %}`) alone on a line leave no empty line behind, and line
statements start with `%%` (inja's default `##` is a Markdown heading).

## Usage

```bash
byzantine-cal [(-v | -q)] [--year=<year> [--old] [--output=<file>] [-C <file>]] <yaml_file>
```

Without `--year` the file is only checked. With `--year` the calendar of that year is
generated and rendered as Markdown with the template found through the configuration; the result
goes to stdout or to the file given by `--output`:

```bash
byzantine-cal --year=2026 calendar.yml > calendar.md
byzantine-cal --year=2026 --old calendar.yml           # old calendar: 25.12. -> 7 January
byzantine-cal --year=2026 --output=2026.md calendar.yml
byzantine-cal --year=2026 -C my-config.yaml calendar.yml
```

Exit status: `0` valid, `1` invalid (errors are logged to stderr with line numbers), `2` a file cannot be read (also a directory) or written (the output file, or stdout) or no configuration file or template is found, `3` wrong command line.
