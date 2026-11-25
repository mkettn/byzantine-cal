# byzantine-cal

C++ successor of [byzantine](https://github.com/mkettn/byzantine). Version 0.1
reads a calendar YAML file and checks that all dates, date ranges and rules are
valid. Nothing is generated yet.

## Date syntax

Same idea as byzantine, but written **day first, then month, separated by a dot**:

| Syntax | Meaning |
|---|---|
| `25.12` / `6.1` / `06.01` | Fixed date (leading zero optional). `29.02` is accepted (leap years). |
| `01.02..14.02` | Range of fixed dates. May wrap over New Year (`15.12..05.01`). |
| `e`, `e+39`, `e-7d`, `e-7w` | Relative to Orthodox Easter (days, or weeks with `w`). |
| `e-7w<wed` / `e+1w>sun` | Then move to the previous (`<`) / next (`>`) weekday. |
| `e-49d..e` | Range between Easter-relative dates. |

## File format

```yaml
- "06.01": Theophany                 # date: name
- "25.03":
    name: Annunciation
    rule: no_fish
- "e+39": Ascension
- "01.02..14.02":                    # range: weekday rules
    mon..fri: no_meat
    sat,sun: no_dairy
```

Rules: `no_fast`, `no_meat`, `no_dairy`, `no_fish`, `no_oil`.
Weekdays: `mon tue wed thu fri sat sun`, as `mon..fri` or `wed,fri`.

## Build

Requires CMake >= 3.24 and a C++20 compiler. Dependencies
(yaml-cpp, spdlog, docopt.cpp) are resolved by `cmake/Find<lib>.cmake`: an
installed copy is used if found, otherwise it is fetched with FetchContent.
Use `-DBCAL_FORCE_FETCH=ON` to always fetch.

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

## Usage

```bash
byzantine-cal [-v | -q] <yaml_file>
```

Exit status: `0` valid, `1` invalid (errors are logged with line numbers), `2` file unreadable.
