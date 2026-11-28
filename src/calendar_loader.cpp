#include "calendar_loader.h"

#include <stdexcept>
#include <string>

#include "byzantine_cal_config.h"
#include "day.h"

namespace bcal {

namespace {

std::string to_yaml_line(const YAML::Node& n) {
  YAML::Emitter out;
  out.SetMapFormat(YAML::Flow);
  out.SetSeqFormat(YAML::Flow);
  out << n;
  return out.c_str();
}

class Loader {
 public:
  Loader(YearCalendar& calendar, const std::filesystem::path& file)
      : calendar_(calendar), file_(file) {}

  void load(const YAML::Node& root) {
    if (root.IsNull() || !root.IsDefined())
      fail(root, "file is empty (expected a list of dates, or [])");
    if (!root.IsSequence())
      fail(root, "calendar must be a list (each item starts with \"- \")");
    for (const auto& item : root) {
      if (!item.IsMap()) fail(item, "list item must be \"date: value\"");
      for (const auto& kv : item) {
        if (!kv.first.IsScalar()) fail(kv.first, "date must be text");
        const auto& key = kv.first.Scalar();
        if (key.find("..") != std::string::npos)
          add_range(kv.first, key, kv.second);
        else
          add_day(kv.first, key, kv.second);
      }
    }
  }

 private:
  [[noreturn]] void fail(const YAML::Node& at, const std::string& message) const {
    const int line = at.IsDefined() && !at.IsNull() && !at.Mark().is_null() ? at.Mark().line + 1 : 0;
    std::string where = file_.empty() ? "" : file_.string() + ":";
    if (line > 0) where += (file_.empty() ? "line " : "") + std::to_string(line) + ":";
    throw std::invalid_argument(where.empty() ? message : where + " " + message);
  }

  Day day(const YAML::Node& at, const std::string& text) const {
    const auto parsed = parse_day(text);
    if (!parsed.day) fail(at, "invalid date: " + parsed.error);
    return *parsed.day;
  }

  void check_rule(const YAML::Node& v) const {
    static constexpr const char* kRules[] = {"no_fast", "no_meat", "no_dairy", "no_fish", "no_oil"};
    if (!v.IsScalar()) fail(v, "rule must be a plain value");
    for (const char* rule : kRules)
      if (v.Scalar() == rule) return;
    fail(v, "unknown rule \"" + v.Scalar() + "\" (use no_fast, no_meat, no_dairy, no_fish, no_oil)");
  }

  void add_day(const YAML::Node& at, const std::string& key, const YAML::Node& value) {
    const Day d = day(at, key);
    if (value.IsScalar()) {  // plain name
      if (value.Scalar().empty()) fail(value, "entry for \"" + key + "\" has an empty name");
    } else if (value.IsMap()) {
      const auto name = value["name"];
      if (!name)
        fail(value, "entry for \"" + key + "\" needs a name");
      else if (!name.IsScalar() || name.Scalar().empty())
        fail(name, "name of \"" + key + "\" must be non-empty text");
      for (const auto& kv : value) {
        if (!kv.first.IsScalar()) fail(kv.first, "field names must be text");
        // `rule` is checked; any other field is arbitrary data.
        if (kv.first.Scalar() == "rule") check_rule(kv.second);
      }
    } else {
      fail(at, "entry for \"" + key + "\" must be a name or {name, rule, ...}");
    }
    calendar_.add_day(d, to_yaml_line(value));
  }

  void check_range_rule(const YAML::Node& v) const {
    if (!v.IsMap()) return check_rule(v);
    int rules = 0;
    for (const auto& kv : v)
      if (kv.first.IsScalar() && kv.first.Scalar() == "rule") {
        ++rules;
        check_rule(kv.second);
      }
    if (rules == 0) fail(v, "rule with data needs a \"rule\" field");
  }

  void add_range(const YAML::Node& at, const std::string& key, const YAML::Node& value) {
    const auto pos = key.find("..");
    const auto start_text = key.substr(0, pos), end_text = key.substr(pos + 2);
    if (end_text.find("..") != std::string::npos)
      fail(at, "range \"" + key + "\" has more than one \"..\"");
    const Day start = day(at, start_text), end = day(at, end_text);
    if (!value.IsMap())
      fail(at, "range \"" + key + "\" needs weekday rules, e.g. mon..fri: no_meat");
    if (ends_before_start(start, end))
      fail(at, "range \"" + key + "\" ends before it starts");
    if (value.size() == 0) fail(at, "range \"" + key + "\" has no weekday rules");

    // One rule per weekday key; later keys win.
    for (const auto& wk : value) {
      if (!wk.first.IsScalar()) fail(wk.first, "weekday key must be text");
      std::string err;
      const Weekdays weekdays = parse_weekday_key(wk.first.Scalar(), err);
      if (!err.empty()) fail(wk.first, err);
      check_range_rule(wk.second);
      calendar_.add_range(start, end, weekdays, to_yaml_line(wk.second));
    }
  }

  YearCalendar& calendar_;
  std::filesystem::path file_;
};

}  // namespace

YearCalendar load_year_calendar(const YAML::Node& root, int year, int offset_days,
                                const std::filesystem::path& file) {
  YearCalendar calendar(year, offset_days);
  Loader(calendar, file).load(root);
  return calendar;
}

YearCalendar load_year_calendar_file(const std::filesystem::path& file, int year,
                                     int offset_days) {
  return load_year_calendar(load_yaml_file(file, "calendar file"), year, offset_days, file);
}

}  // namespace bcal
