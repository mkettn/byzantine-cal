#include "template_backend.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

#include <inja/inja.hpp>

namespace bcal {

namespace {

constexpr std::array<const char*, 12> kMonths = {
    "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",   "September", "October", "November", "December"};
constexpr std::array<const char*, 7> kWeekdays = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

nlohmann::json day_to_json(const CalendarDay& d) {
  const int y = static_cast<int>(d.date.year());
  const unsigned m = static_cast<unsigned>(d.date.month());
  const unsigned dd = static_cast<unsigned>(d.date.day());
  char iso[16];
  std::snprintf(iso, sizeof iso, "%04d-%02u-%02u", y, m, dd);
  const int weekday = static_cast<int>(d.weekday().iso_encoding());
  return {{"date", iso},
          {"year", y},
          {"month", m},
          {"day", dd},
          {"weekday", weekday},
          {"weekday_name", kWeekdays[static_cast<std::size_t>(weekday - 1)]},
          {"has_rule", d.has_rule()},
          {"data", d.data.value_or("")}};
}

std::string iso_date(std::chrono::year_month_day d) {
  return day_to_json({d, std::nullopt})["date"];
}

}  // namespace

std::string escape_markdown(const std::string& text) {
  std::string out;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') continue;
    if (c == '\n' || c == '\r') {
      out += "<br>";
    } else {
      if (std::string_view("\\`*_{}[]()<>#+-.!|~&").find(c) != std::string_view::npos)
        out += '\\';
      out += c;
    }
  }
  return out;
}

nlohmann::json calendar_to_json(const YearCalendar& calendar) {
  nlohmann::json days = nlohmann::json::array();
  nlohmann::json months = nlohmann::json::array();
  for (std::size_t m = 0; m < kMonths.size(); ++m)
    months.push_back({{"number", m + 1},
                      {"name", kMonths[m]},
                      {"rule_count", 0},
                      {"days", nlohmann::json::array()}});

  for (const auto& d : calendar) {
    auto j = day_to_json(d);
    auto& month = months[static_cast<unsigned>(d.date.month()) - 1];
    month["days"].push_back(j);
    if (d.has_rule()) month["rule_count"] = month["rule_count"].get<int>() + 1;
    days.push_back(std::move(j));
  }
  return {{"year", calendar.year()},
          {"offset_days", calendar.offset_days()},
          {"pascha", iso_date(calendar.pascha())},
          {"days", std::move(days)},
          {"months", std::move(months)}};
}

std::string render_template(const YearCalendar& calendar,
                            const std::string& template_text) {
  try {
    inja::Environment env;
    env.set_line_statement("%%");  // inja's default "##" is a Markdown heading
    env.set_trim_blocks(true);
    env.set_lstrip_blocks(true);
    env.add_callback("escape_markdown", 1, [](inja::Arguments& args) {
      return escape_markdown(args.at(0)->get<std::string>());
    });
    env.add_callback("replace", 3, [](inja::Arguments& args) {
      std::string text = args.at(0)->get<std::string>();
      const auto from = args.at(1)->get<std::string>();
      const auto to = args.at(2)->get<std::string>();
      if (!from.empty())
        for (std::size_t pos = 0; (pos = text.find(from, pos)) != std::string::npos;
             pos += to.size())
          text.replace(pos, from.size(), to);
      return text;
    });
    return env.render(template_text, calendar_to_json(calendar));
  } catch (const std::exception& e) {
    // Invalid templates (inja::InjaError), JSON errors and anything a template
    // can cause, such as running out of memory with range(99999999999).
    throw TemplateError(e.what());
  }
}

TemplateBackend::TemplateBackend(const std::string& template_path) {
  std::ifstream in(template_path);
  if (!in || !std::filesystem::is_regular_file(template_path))
    throw TemplateFileError("cannot read template " + template_path);
  std::stringstream ss;
  ss << in.rdbuf();
  template_ = ss.str();
}

}  // namespace bcal
