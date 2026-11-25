#include "validator.h"

#include <array>
#include <sstream>

#include "day.h"

namespace bcal {

namespace {

constexpr std::array<const char*, 5> kRules = {"no_fast", "no_meat", "no_dairy",
                                               "no_fish", "no_oil"};

int line_of(const YAML::Node& n) {
  return n.Mark().is_null() ? 0 : n.Mark().line + 1;
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

// "mon..fri", "wed,fri", "sat"
void check_weekday_key(Report& r, const YAML::Node& at, const std::string& key) {
  for (const auto& item : split(key, ',')) {
    auto part = trim(item);
    auto pos = part.find("..");
    if (pos == std::string::npos) {
      if (!parse_weekday(part))
        add(r, at, "unknown weekday \"" + part + "\" (use mon..sun)");
    } else {
      auto a = trim(part.substr(0, pos)), b = trim(part.substr(pos + 2));
      if (!parse_weekday(a) || !parse_weekday(b))
        add(r, at, "invalid weekday range \"" + part + "\" (e.g. mon..fri)");
    }
  }
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
  for (const auto& kv : value) {
    if (!kv.first.IsScalar()) {
      add(r, kv.first, "weekday key must be text");
      continue;
    }
    check_weekday_key(r, kv.first, kv.first.Scalar());
    check_rule(r, kv.second, kv.second);
  }
}

void check_fixed(Report& r, const YAML::Node& key_node, const std::string& key,
                 const YAML::Node& value) {
  check_day(r, key_node, key);
  if (value.IsScalar()) return;  // plain name
  if (value.IsMap()) {
    if (!value["name"]) add(r, value, "entry for \"" + key + "\" needs a name");
    for (const auto& kv : value) {
      auto k = kv.first.as<std::string>("");
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
    return validate(YAML::LoadFile(path));
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
