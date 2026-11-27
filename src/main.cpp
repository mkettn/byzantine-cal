#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <docopt/docopt.h>

#include <charconv>
#include <cstdio>
#include <map>
#include <string>

#include <yaml-cpp/yaml.h>

#include "pascha.h"
#include "validator.h"
#include "calendar_loader.h"

static bool parse_int(const std::string& text, int& out) {
  auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
  return ec == std::errc() && ptr == text.data() + text.size();
}

static const char USAGE[] = R"(byzantine-cal - check a fasting calendar file and generate the calendar of a year.

Dates are written DD.MM (e.g. 25.12), ranges as DD.MM..DD.MM,
and Pascha-relative days as p, p+39, p-7w, p-7w<wed.

Usage:
  byzantine-cal [(-v | -q)] [--year=<year> [--old]] <yaml_file>
  byzantine-cal (-h | --help)
  byzantine-cal --version

Options:
  -v --verbose  Show debug output.
  -q --quiet    Only show errors.
  -y <year>, --year=<year>
                Generate the calendar of this year and print
                every day that has a rule: "YYYY-MM-DD<TAB>data".
  --old         Old (Julian) calendar: shift all fixed (DD.MM) dates by
                13 days, e.g. 25.12 becomes 7 January. Pascha dates are
                not shifted. Needs --year.
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

  int year = 0;
  const bool generate = static_cast<bool>(args["--year"]);
  if (generate ? !parse_int(args["--year"].asString(), year) ||
                     year < bcal::kMinYear || year > bcal::kMaxYear
               : args["--old"].asBool()) {
    std::fprintf(stderr, generate ? "year must be a whole number in 1..9999\n"
                                  : "--old needs --year\n");
    return 3;
  }
  const int offset_days =
      args["--old"].asBool() ? bcal::kJulianGregorianOffsetDays : 0;

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
    for (const auto& day : bcal::load_year_calendar(root, year, offset_days))
      if (day.has_rule())
        std::printf("%04d-%02u-%02u\t%s\n", static_cast<int>(day.date.year()),
                    static_cast<unsigned>(day.date.month()),
                    static_cast<unsigned>(day.date.day()), day.data->c_str());
  }
  return 0;
}
