# byzantine-cal

Orthodox fasting calendar tool. It reads a calendar file with dates and date ranges, checks it, and
generates the calendar of a year from it as Markdown or iCalendar, with a template.

## Build

Requires CMake >= 3.24 and a C++20 compiler. The dependencies (yaml-cpp, spdlog, docopt.cpp,
nlohmann_json, inja) are resolved by `cmake/Find<lib>.cmake`: an installed copy is used if found,
otherwise it is fetched with FetchContent.

Installed libraries of these versions **or newer** are accepted. If none is found, exactly these
versions are fetched. They are the ones shipped in Ubuntu 24.04 LTS (`libyaml-cpp-dev`,
`libspdlog-dev`, `libdocopt-dev`, `nlohmann-json3-dev`), except for inja, which Ubuntu does not
package:

| Library | Version |
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

The program goes to `<prefix>/bin`. The configuration `config/config.yaml`, the templates in `tpl/`
and the language files in `lang/` go to `<prefix>/lib/byzantine-cal`. The prefix is compiled in,
so it has to be set when configuring, not when installing.

## Run

```bash
byzantine-cal [options] <yaml_file>
```

Without `--year` the calendar file is only checked. With `--year` the calendar of that year is
generated and rendered as Markdown (`--format=html`: HTML, `--format=ics`: iCalendar). The result goes to stdout, or to the file given by `--output`.

```bash
byzantine-cal calendar.yml                                  # check the file
byzantine-cal --year=2026 calendar.yml > calendar.md
byzantine-cal --year=2026 --old calendar.yml                # old calendar: 25.12. -> 7 January
byzantine-cal --year=2026 --output=2026.md calendar.yml
byzantine-cal --year=2026 -C my-config.yaml calendar.yml    # another configuration
byzantine-cal --year=2026 --format=html calendar.yml > 2026.html
byzantine-cal --year=2026 --format=ics calendar.yml > 2026.ics  # iCalendar for a calendar app
byzantine-cal --year=2026 --lang=de_DE calendar.yml         # German month and weekday names
```

| Option | |
|---|---|
| `-h`, `--help`, `--version` | Show the help, or the version. |
| `-v`, `-q` | Show debug output, or only errors (not both). |
| `-y`, `--year=<year>` | Generate the calendar of this year. |
| `--old` | The calendar is in the old (Julian) calendar: shift the fixed dates by 13 days. Needs `--year`. |
| `-o`, `--output=<file>` | Write the result to this file instead of stdout. Needs `--year`. |
| `-f`, `--format=<format>` | `markdown` (default), `html` or `ics`. Needs `--year`. |
| `-C <file>` | The configuration file. Needs `--year`. |
| `-l`, `--lang=<language>` | The language, overriding the configuration. Needs `--year`. |

Without installing, run it from the build directory with a configuration that points at the sources,
see [Running without installing](docs/configuration.md#running-without-installing).

Exit status: `0` success, `1` the calendar file or the configuration is invalid or a template fails
(errors are logged to stderr, with line numbers), `2` a file cannot be read (also a directory) or
written (the output file, or stdout) or no configuration file, template or language file is found,
`3` wrong command line.

## Documentation

- [Dates and date ranges](docs/dates.md): the syntax, the calendar file and how a year is generated
- [Configuration](docs/configuration.md): the configuration file and where it is looked for
- [Templates](docs/templates.md): the templates and the data they get
- [Languages](docs/language.md): language files and fallbacks
