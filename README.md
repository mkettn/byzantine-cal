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

`YearCalendar` (`src/year_calendar.h`) generates the calendar of one year, from 1 January
to 31 December. Every day is a `CalendarDay` with its date and, if a rule applies, the
rule's data as a single-line YAML string (e.g. `no_meat` or
`{name: Nativity, rule: no_fish}`). Days can be queried or iterated:

```cpp
YAML::Node root = YAML::LoadFile("calendar.yml");
bcal::YearCalendar cal(root, 2026, /*offset_days=*/13);

const bcal::CalendarDay& d = cal.at(1, 7);   // or cal.at(2026y / January / 7)
if (d.has_rule()) std::cout << *d.data;

for (const bcal::CalendarDay& day : cal) { /* 365 or 366 days, in order */ }
```

- Rules are applied in file order; where several apply to a day, the later one wins.
- The offset is added to every fixed (`DD.MM`) date and leaps over year boundaries:
  with an offset of 13, 25.12. of the previous year falls on 7 January. Pascha-relative
  dates are not shifted. Weekday rules use the weekday of the shifted date.
- 29.02. only exists in leap years; in other years the entry has no effect.
- Years 2026..2099 (the Julian calendar is 13 days behind the Gregorian one until 2100) and offsets of -365..365 are supported.

## Usage

```bash
byzantine-cal [(-v | -q)] [--year=<year> [--offset=<days>]] <yaml_file>
```

Without `--year` the file is only checked. With `--year` the calendar of that year is
generated and every day that has a rule is printed as `YYYY-MM-DD<TAB>data`:

```bash
byzantine-cal --year=2026 calendar.yml
byzantine-cal --year=2026 --offset=13 calendar.yml   # 25.12. -> 7 January
```

Exit status: `0` valid, `1` invalid (errors are logged to stderr with line numbers), `2` file unreadable (or a directory), `3` wrong command line.
