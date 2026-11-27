#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <docopt/docopt.h>

#include <cstdio>
#include <map>
#include <string>

#include <yaml-cpp/yaml.h>

#include "validator.h"

static const char USAGE[] = R"(byzantine-cal - check a fasting calendar file.

Dates are written DD.MM (e.g. 25.12), ranges as DD.MM..DD.MM,
and Pascha-relative days as p, p+39, p-7w, p-7w<wed.

Usage:
  byzantine-cal [(-v | -q)] <yaml_file>
  byzantine-cal (-h | --help)
  byzantine-cal --version

Options:
  -v --verbose  Show debug output.
  -q --quiet    Only show errors.
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

  const auto path = args["<yaml_file>"].asString();
  spdlog::debug("reading {}", path);

  bcal::Report report;
  try {
    report = bcal::validate_file(path);
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
  return 0;
}
