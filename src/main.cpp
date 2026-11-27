#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <docopt/docopt.h>

#include <charconv>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include <yaml-cpp/yaml.h>

#include "builtin_templates.h"
#include "calendar_loader.h"
#include "pascha.h"
#include "template_backend.h"
#include "validator.h"

static bool parse_int(const std::string& text, int& out) {
  auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
  return ec == std::errc() && ptr == text.data() + text.size();
}

static const char USAGE[] = R"(byzantine-cal - check a fasting calendar file and generate the calendar of a year.

Dates are written DD.MM (e.g. 25.12), ranges as DD.MM..DD.MM,
and Pascha-relative days as p, p+39, p-7w, p-7w<wed.

Usage:
  byzantine-cal [(-v | -q)] [--year=<year> [--old] [--template=<file>] [--output=<file>]]
                <yaml_file>
  byzantine-cal (-h | --help)
  byzantine-cal --version

Options:
  -v --verbose  Show debug output.
  -q --quiet    Only show errors.
  -y <year>, --year=<year>
                Generate the calendar of this year and render it with a
                template (Markdown unless --template is given).
  --old         Old (Julian) calendar: shift all fixed (DD.MM) dates by
                13 days, e.g. 25.12 becomes 7 January. Pascha dates are
                not shifted. Needs --year.
  -t <file>, --template=<file>
                Render with this inja template instead of the built-in
                Markdown one. Needs --year.
  -o <file>, --output=<file>
                Write the result to this file instead of stdout.
                Needs --year.
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
               : args["--old"].asBool() || args["--template"] ||
                     args["--output"]) {
    std::fprintf(stderr, generate ? "year must be a whole number in 1..9999\n"
                                  : "--old, --template and --output need --year\n");
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

  if (!generate) return 0;

  std::string tpl = bcal::kMarkdownTemplate;
  if (args["--template"]) {
    const auto tpl_path = args["--template"].asString();
    std::ifstream in(tpl_path);
    if (!in) {
      spdlog::error("cannot read template {}", tpl_path);
      return 2;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    tpl = ss.str();
  }

  std::string text;
  try {
    text = bcal::render_template(
        bcal::load_year_calendar(root, year, offset_days), tpl);
  } catch (const bcal::TemplateError& e) {
    spdlog::error("template error: {}", e.what());
    return 1;
  }

  if (args["--output"]) {
    const auto out_path = args["--output"].asString();
    std::ofstream out(out_path);
    if (!(out << text)) {
      spdlog::error("cannot write {}", out_path);
      return 2;
    }
  } else {
    std::fputs(text.c_str(), stdout);
  }
  return 0;
}
