#include <spdlog/spdlog.h>

#include <docopt/docopt.h>

#include <yaml-cpp/yaml.h>

#include "validator.h"

static const char USAGE[] = R"(byzantine-cal - check a fasting calendar file.

Dates are written DD.MM (e.g. 25.12), ranges as DD.MM..DD.MM,
and Easter-relative days as e, e+39, e-7w, e-7w<wed.

Usage:
  byzantine-cal [options] <yaml_file>
  byzantine-cal (-h | --help)
  byzantine-cal --version

Options:
  -v --verbose  Show debug output.
  -q --quiet    Only show errors.
  -h --help     Show this help.
  --version     Show version.

Exit status: 0 = valid, 1 = invalid, 2 = file could not be read.
)";

int main(int argc, char** argv) {
  auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true,
                             std::string("byzantine-cal ") + BCAL_VERSION);

  if (args["--verbose"].asBool())
    spdlog::set_level(spdlog::level::debug);
  else if (args["--quiet"].asBool())
    spdlog::set_level(spdlog::level::err);

  const auto path = args["<yaml_file>"].asString();
  spdlog::debug("reading {}", path);

  bcal::Report report;
  try {
    report = bcal::validate_file(path);
  } catch (const YAML::BadFile&) {
    spdlog::error("cannot read file: {}", path);
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
