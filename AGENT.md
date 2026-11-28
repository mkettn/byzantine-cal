# Agent guidelines for byzantine-cal

C++20 command line tool that checks a fasting calendar file and generates the calendar of a year from it
with templates.

## Folders

- `src/` the code: the library (`bcal`) and `main.cpp`, the command line
- `tpl/` templates (inja), one per output format
- `lang/` language files: translations of the generated files
- `config/` the configuration that is installed with the program
- `docs/` the documentation, the one place for it
- `cmake/` one `Find<lib>.cmake` per dependency: an installed copy, else FetchContent
- `tests/` `test_main.cpp` (unit tests), fixtures, and the scripts of the command line tests (registered in `CMakeLists.txt`)
- `.github/` CI

## Rules

- The user interface of the application (command line, messages, logs) is English. Translation is
  for the generated files only.
- Documentation is in `docs/`. `README.md` only has build, install and run instructions (the options and the exit status included). Comments in `src/`
  are 1-2 lines. Less is more.
- Don't be overly defensive when programming.
- Show messages with spdlog. Signal errors with exceptions.
- No environment variables.
- A new setting is a new key in the config file, and a new command line parameter if suitable.
- Keep PRs short: at most 300 new lines is the goal, more is ok. At about 500, consider two PRs.
- When a PR is implemented: start a review agent, fix the findings, do another round, and mark the PR
  ready if it is fine.
