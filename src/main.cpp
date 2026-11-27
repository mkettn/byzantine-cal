#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <docopt/docopt.h>

#include <charconv>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

#include <yaml-cpp/yaml.h>

#include "pascha.h"
#include "validator.h"
#include "year_calendar.h"

static bool parse_int(const std::string& text, int& out) {
  auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
  return ec == std::errc() && ptr == text.data() + text.size();
}

static const char USAGE[] = R"(byzantine-cal - check a fasting calendar file and generate the calendar of a year.

Dates are written DD.MM (e.g. 25.12), ranges as DD.MM..DD.MM,
and Pascha-relative days as p, p+39, p-7w, p-7w<wed.

Usage:
  byzantine-cal [(-v | -q)] [--year=<year> [--offset=<days>]] <yaml_file>
  byzantine-cal (-h | --help)
  byzantine-cal --version

Options:
  -v --verbose  Show debug output.
  -q --quiet    Only show errors.
  -y <year>, --year=<year>
                Generate the calendar of this year and print
                every day that has a rule: "YYYY-MM-DD<TAB>data".
  -o <days>, --offset=<days>
                Shift all fixed (DD.MM) dates by this many days, e.g. 13
                moves 25.12 to 7 January. Pascha dates are not shifted.
                Needs --year; default is 0.
  -h --help     Show this help.
  --version     Show version.

Exit status: 0 = valid, 1 = invalid, 2 = file could not be read,
3 = wrong command line.
)";

int main(int argc, char** argv) {
  spdlog::set_default_logger(spdlog::stderr_color_mt("bcal"));
  std::map<std::string, docopt::value> args;
  try {
    args = docopt::docopt_parse(USAGE, {argv + 1, argv + argc}, true, true);
  } catch (const docopt::DocoptExitHelp&) {
    std::puts(USAGE);
    return 0;
  } catch (const docopt::DocoptExitVersion&) {
    std::puts("byzantine-cal " BCAL_VERSION);
    return 0;
  } catch (const docopt::DocoptArgumentError& e) {
    std::fprintf(stderr, "%s\n\n%s", e.what(), USAGE);
    return 3;
  } catch (const docopt::DocoptLanguageError& e) {
    std::fprintf(stderr, "%s\n", e.what());
    return 3;
  }

  if (args["--verbose"].asBool())
    spdlog::set_level(spdlog::level::debug);
  else if (args["--quiet"].asBool())
    spdlog::set_level(spdlog::level::err);

  int year = 0, offset = 0;
  const bool generate = static_cast<bool>(args["--year"]);
  if (!generate && args["--offset"]) {
    std::fprintf(stderr, "--offset needs --year\n\n%s", USAGE);
    return 3;
  }
  if (generate) {
    if (!parse_int(args["--year"].asString(), year) ||
        (args["--offset"] &&
         !parse_int(args["--offset"].asString(), offset))) {
      std::fprintf(stderr, "year and offset must be whole numbers\n");
      return 3;
    }
    if (year < bcal::kMinYear || year > bcal::kMaxYear || offset < -365 || offset > 365) {
      std::fprintf(stderr,
                   "year must be 1..9999 and offset -365..365\n");
      return 3;
    }
  }

  const auto path = args["<yaml_file>"].asString();
  spdlog::debug("reading {}", path);

  bcal::Report report;
  YAML::Node root;
  try {
    root = bcal::load_file(path, report);
    if (report.ok()) report = bcal::validate(root);
  } catch (const std::exception& e) {
    spdlog::error("cannot read {}: {}", path, e.what());
    return 2;
  }

  for (const auto& e : report.errors) {
    if (e.line > 0)
      spdlog::error("{}:{}: {}", path, e.line, e.message);
    else
      spdlog::error("{}: {}", path, e.message);
  }
  if (!report.ok()) {
    spdlog::error("{} problem(s) found", report.errors.size());
    return 1;
  }
  spdlog::info("{}: OK ({} entries)", path, report.entries);

  if (generate) {
    bcal::YearCalendar calendar(root, year, offset);
    int with_rule = 0;
    for (const auto& day : calendar) {
      if (!day.has_rule()) continue;
      ++with_rule;
      std::printf("%04d-%02u-%02u\t%s\n", static_cast<int>(day.date.year()),
                  static_cast<unsigned>(day.date.month()),
                  static_cast<unsigned>(day.date.day()), day.data->c_str());
    }
    spdlog::debug("{}: {} of {} days have a rule", year, with_rule,
                  calendar.size());
  }
  return 0;
}
