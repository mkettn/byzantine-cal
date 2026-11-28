#include "template_backend.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

#include <inja/inja.hpp>

namespace bcal {

namespace {

nlohmann::json day_to_json(const CalendarDay& d, const Language& language) {
  const int y = static_cast<int>(d.date.year());
  const unsigned m = static_cast<unsigned>(d.date.month());
  const unsigned dd = static_cast<unsigned>(d.date.day());
  char iso[16], basic[16];
  std::snprintf(iso, sizeof iso, "%04d-%02u-%02u", y, m, dd);
  std::snprintf(basic, sizeof basic, "%04d%02u%02u", y, m, dd);
  const int weekday = static_cast<int>(d.weekday().iso_encoding());
  return {{"date", iso},
          {"date_basic", basic},
          {"year", y},
          {"month", m},
          {"day", dd},
          {"weekday", weekday},
          {"weekday_name", language.weekday(static_cast<unsigned>(weekday))},
          {"has_rule", d.has_rule()},
          {"data", d.data.value_or("")}};
}

}  // namespace

std::string escape_markdown(const std::string& text) {
  std::string out;
  for (const char c : text) {
    if (c == '\r') continue;
    if (c == '\n') {
      out += "<br>";
      continue;
    }
    if (std::string_view("\\`*_{}[]()<>#+-.!|~&").find(c) != std::string_view::npos)
      out += '\\';
    out += c;
  }
  return out;
}

std::string escape_html(const std::string& text) {
  std::string out;
  for (const char c : text) {
    switch (c) {
      case '\r': break;
      case '\n': out += "<br>"; break;
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c;
    }
  }
  return out;
}

std::string escape_ics(const std::string& text) {
  std::string out;
  for (const char c : text) {
    if (c == '\r') continue;
    if (c == '\n') {
      out += "\\n";
      continue;
    }
    if (c == '\\' || c == ';' || c == ',') out += '\\';
    out += c;
  }
  return out;
}

std::string fold_ics(const std::string& text) {
  constexpr size_t kMax = 75;  // octets per line, without the CRLF
  std::string out;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    size_t start = pos;
    const size_t next = end + 1;
    if (end > start && text[end - 1] == '\r') --end;  // a template saved with CRLF
    size_t limit = kMax;
    while (end - start > limit) {
      size_t cut = start + limit;
      size_t back = cut;  // do not cut inside a UTF-8 character
      while (back > start + 1 && (static_cast<unsigned char>(text[back]) & 0xC0) == 0x80) --back;
      if (back > start + 1) cut = back;  // invalid UTF-8: cut anyway
      out.append(text, start, cut - start).append("\r\n ");
      start = cut;
      limit = kMax - 1;  // the leading space counts
    }
    out.append(text, start, end - start).append("\r\n");
    pos = next;
  }
  return out;
}

nlohmann::json calendar_to_json(const YearCalendar& calendar, const Language& language) {
  nlohmann::json days = nlohmann::json::array();
  nlohmann::json months = nlohmann::json::array();
  for (unsigned m = 1; m <= 12; ++m)
    months.push_back({{"number", m},
                      {"name", language.month(m)},
                      {"rule_count", 0},
                      {"days", nlohmann::json::array()}});

  for (const auto& d : calendar) {
    auto j = day_to_json(d, language);
    auto& month = months[static_cast<unsigned>(d.date.month()) - 1];
    month["days"].push_back(j);
    if (d.has_rule()) month["rule_count"] = month["rule_count"].get<int>() + 1;
    days.push_back(std::move(j));
  }
  return {{"year", calendar.year()},
          {"offset_days", calendar.offset_days()},
          {"pascha", day_to_json({calendar.pascha(), std::nullopt, {}}, language)["date"]},
          {"ui", language.ui()},
          {"days", std::move(days)},
          {"months", std::move(months)}};
}

std::string render_template(const YearCalendar& calendar, const Language& language,
                            const std::string& template_text) {
  try {
    inja::Environment env;
    env.set_line_statement("%%");  // inja's default "##" is a Markdown heading
    env.set_trim_blocks(true);
    env.set_lstrip_blocks(true);
    env.add_callback("escape_markdown", 1, [](inja::Arguments& args) {
      return escape_markdown(args.at(0)->get<std::string>());
    });
    env.add_callback("escape_html", 1, [](inja::Arguments& args) {
      return escape_html(args.at(0)->get<std::string>());
    });
    env.add_callback("escape_ics", 1, [](inja::Arguments& args) {
      return escape_ics(args.at(0)->get<std::string>());
    });
    return env.render(template_text, calendar_to_json(calendar, language));
  } catch (const std::exception& e) {
    // Invalid templates (inja::InjaError), JSON errors and anything a template
    // can cause, such as running out of memory with range(99999999999).
    throw TemplateError(e.what());
  }
}

TemplateBackend::TemplateBackend(const std::filesystem::path& template_path) {
  std::ifstream in(template_path);
  if (!in || !std::filesystem::is_regular_file(template_path))
    throw TemplateFileError("cannot read template " + template_path.string());
  std::stringstream ss;
  ss << in.rdbuf();
  template_ = ss.str();
}

}  // namespace bcal
