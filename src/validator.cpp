#include "validator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <set>
#include <sstream>

#include "day.h"

namespace bcal {

namespace {

constexpr std::array<const char*, 5> kRules = {"no_fast", "no_meat", "no_dairy",
                                               "no_fish", "no_oil"};

int line_of(const YAML::Node& n) {
  if (!n.IsDefined() || n.IsNull() || n.Mark().is_null()) return 0;
  return n.Mark().line + 1;
}

void add(Report& r, const YAML::Node& n, std::string msg) {
  r.errors.push_back({line_of(n), std::move(msg)});
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string part;
  while (std::getline(ss, part, sep)) out.push_back(part);
  if (!s.empty() && s.back() == sep) out.emplace_back();
  return out;
}

std::string trim(const std::string& s) {
  auto b = s.find_first_not_of(" \t");
  if (b == std::string::npos) return {};
  return s.substr(b, s.find_last_not_of(" \t") - b + 1);
}

void check_day(Report& r, const YAML::Node& at, const std::string& text) {
  auto res = parse_day(text);
  if (!res.day) add(r, at, "invalid date: " + res.error);
}

void check_rule(Report& r, const YAML::Node& at, const YAML::Node& v) {
  if (!v.IsScalar()) {
    add(r, at, "rule must be a plain value");
    return;
  }
  if (!is_valid_rule(v.Scalar()))
    add(r, at, "unknown rule \"" + v.Scalar() +
                   "\" (use no_fast, no_meat, no_dairy, no_fish, no_oil)");
}

// Weekday key grammar: either one range "mon..fri" (wrapping allowed, e.g.
// "sat..mon") or a comma list of single days "wed,fri" / "sat". Lowercase only.
// Returns the weekdays (0 = mon .. 6 = sun) it covers; sets err on failure.
std::set<int> weekdays_of(const std::string& key, std::string& err) {
  std::set<int> days;
  auto one = [&](const std::string& name) -> int {
    if (name.empty()) return -1;
    auto lc = name;
    std::transform(lc.begin(), lc.end(), lc.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (lc != name) return -1;  // must be lowercase
    return parse_weekday(name).value_or(-1);
  };
  if (key.empty()) {
    err = "empty weekday key";
    return days;
  }
  auto pos = key.find("..");
  if (pos != std::string::npos) {
    int a = one(key.substr(0, pos)), b = one(key.substr(pos + 2));
    if (a < 0 || b < 0 || key.find(',') != std::string::npos) {
      err = "invalid weekday range \"" + key +
            "\" (write e.g. mon..fri, lowercase, no spaces or commas)";
      return days;
    }
    for (int d = a;; d = (d + 1) % 7) {
      days.insert(d);
      if (d == b) break;
    }
    return days;
  }
  for (const auto& item : split(key, ',')) {
    int d = one(trim(item));
    if (d < 0) {
      err = "unknown weekday \"" + trim(item) + "\" in \"" + key +
            "\" (use lowercase mon tue wed thu fri sat sun)";
      return {};
    }
    days.insert(d);
  }
  return days;
}

void check_range(Report& r, const YAML::Node& key_node, const std::string& key,
                 const YAML::Node& value) {
  auto pos = key.find("..");
  auto start = key.substr(0, pos), end = key.substr(pos + 2);
  if (end.find("..") != std::string::npos) {
    add(r, key_node, "range \"" + key + "\" has more than one \"..\"");
    return;
  }
  check_day(r, key_node, start);
  check_day(r, key_node, end);

  if (!value.IsMap()) {
    add(r, key_node,
        "range \"" + key + "\" needs weekday rules, e.g. mon..fri: no_meat");
    return;
  }
  // Easter-relative range that ends before it starts can never match.
  auto ds = parse_day(start), de = parse_day(end);
  if (ds.day && de.day && ds.day->kind == Day::Kind::Easter &&
      de.day->kind == Day::Kind::Easter && !ds.day->prev_weekday &&
      !ds.day->next_weekday && !de.day->prev_weekday && !de.day->next_weekday &&
      de.day->offset_days < ds.day->offset_days)
    add(r, key_node, "range \"" + key + "\" ends before it starts");

  if (value.size() == 0)
    add(r, key_node, "range \"" + key + "\" has no weekday rules");
  std::set<int> seen;
  for (const auto& kv : value) {
    if (!kv.first.IsScalar()) {
      add(r, kv.first, "weekday key must be text");
      continue;
    }
    std::string err;
    auto days = weekdays_of(kv.first.Scalar(), err);
    if (!err.empty())
      add(r, kv.first, err);
    else
      for (int d : days)
        if (!seen.insert(d).second) {
          add(r, kv.first, "weekday listed twice in range \"" + key +
                               "\" (in \"" + kv.first.Scalar() + "\")");
          break;
        }
    check_rule(r, kv.second, kv.second);
  }
}

void check_fixed(Report& r, const YAML::Node& key_node, const std::string& key,
                 const YAML::Node& value) {
  check_day(r, key_node, key);
  if (value.IsScalar()) {  // plain name
    if (value.Scalar().empty())
      add(r, value, "entry for \"" + key + "\" has an empty name");
    return;
  }
  if (value.IsMap()) {
    auto name = value["name"];
    if (!name)
      add(r, value, "entry for \"" + key + "\" needs a name");
    else if (!name.IsScalar() || name.Scalar().empty())
      add(r, name, "name of \"" + key + "\" must be non-empty text");
    for (const auto& kv : value) {
      if (!kv.first.IsScalar()) {
        add(r, kv.first, "field names must be text");
        continue;
      }
      const auto& k = kv.first.Scalar();
      if (k == "rule")
        check_rule(r, kv.second, kv.second);
      else if (k != "name")
        add(r, kv.first, "unknown field \"" + k + "\" (use name, rule)");
    }
    return;
  }
  add(r, key_node, "entry for \"" + key + "\" must be a name or {name, rule}");
}

}  // namespace

bool is_valid_rule(const std::string& rule) {
  for (auto* k : kRules)
    if (rule == k) return true;
  return false;
}

Report validate(const YAML::Node& root) {
  Report r;
  if (root.IsNull() || !root.IsDefined()) {
    add(r, root, "file is empty (expected a list of dates, or [])");
    return r;
  }
  if (!root.IsSequence()) {
    add(r, root, "calendar must be a list (each item starts with \"- \")");
    return r;
  }
  for (const auto& item : root) {
    if (!item.IsMap()) {
      add(r, item, "list item must be \"date: value\"");
      continue;
    }
    for (const auto& kv : item) {
      if (!kv.first.IsScalar()) {
        add(r, kv.first, "date must be text");
        continue;
      }
      ++r.entries;
      const auto& key = kv.first.Scalar();
      if (key.find("..") != std::string::npos)
        check_range(r, kv.first, key, kv.second);
      else
        check_fixed(r, kv.first, key, kv.second);
    }
  }
  return r;
}

Report validate_file(const std::string& path) {
  try {
    namespace fs = std::filesystem;
    if (!fs::is_regular_file(path)) throw YAML::BadFile(path);
    auto docs = YAML::LoadAllFromFile(path);
    if (docs.empty()) return validate(YAML::Node());
    if (docs.size() > 1) {
      Report r;
      r.errors.push_back({0, "file has " + std::to_string(docs.size()) +
                                 " YAML documents; use exactly one (no \"---\" separators)"});
      return r;
    }
    return validate(docs.front());
  } catch (const YAML::BadFile&) {
    throw;
  } catch (const YAML::Exception& e) {
    Report r;
    r.errors.push_back({e.mark.is_null() ? 0 : e.mark.line + 1,
                        std::string("YAML syntax error: ") + e.msg});
    return r;
  }
}

}  // namespace bcal
