#include "validator.h"

#include <array>
#include <filesystem>
#include <set>

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

// A weekday rule is a rule name ("no_meat") or a map with a required `rule`
// and arbitrary further fields ({rule: no_meat, note: ...}).
void check_range_rule(Report& r, const YAML::Node& v) {
  if (v.IsMap()) {
    if (!v["rule"])
      add(r, v, "rule with data needs a \"rule\" field");
    else
      check_rule(r, v["rule"], v["rule"]);
    return;
  }
  check_rule(r, v, v);
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
  // Pascha-relative range that ends before it starts can never match.
  auto ds = parse_day(start), de = parse_day(end);
  if (ds.day && de.day && ds.day->kind == Day::Kind::Pascha &&
      de.day->kind == Day::Kind::Pascha && !ds.day->prev_weekday &&
      !ds.day->next_weekday && !de.day->prev_weekday && !de.day->next_weekday &&
      de.day->offset_days < ds.day->offset_days)
    add(r, key_node, "range \"" + key + "\" ends before it starts");

  if (value.size() == 0)
    add(r, key_node, "range \"" + key + "\" has no weekday rules");
  for (const auto& kv : value) {
    if (!kv.first.IsScalar()) {
      add(r, kv.first, "weekday key must be text");
      continue;
    }
    std::string err;
    parse_weekday_key(kv.first.Scalar(), err);  // validation only; later keys win
    if (!err.empty()) add(r, kv.first, err);
    check_range_rule(r, kv.second);
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
      // `rule` is checked; any other field is arbitrary data.
      if (kv.first.Scalar() == "rule") check_rule(r, kv.second, kv.second);
    }
    return;
  }
  add(r, key_node,
      "entry for \"" + key + "\" must be a name or {name, rule, ...}");
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

YAML::Node load_file(const std::string& path, Report& report) {
  try {
    namespace fs = std::filesystem;
    if (!fs::is_regular_file(path)) throw YAML::BadFile(path);
    auto docs = YAML::LoadAllFromFile(path);
    if (docs.empty()) return YAML::Node();
    if (docs.size() > 1) {
      report.errors.push_back(
          {0, "file has " + std::to_string(docs.size()) +
                  " YAML documents; use exactly one (no \"---\" separators)"});
      return YAML::Node();
    }
    return docs.front();
  } catch (const YAML::BadFile&) {
    throw;
  } catch (const YAML::Exception& e) {
    report.errors.push_back({e.mark.is_null() ? 0 : e.mark.line + 1,
                             std::string("YAML syntax error: ") + e.msg});
    return YAML::Node();
  }
}

Report validate_file(const std::string& path) {
  Report r;
  auto root = load_file(path, r);
  if (!r.ok()) return r;
  return validate(root);
}

}  // namespace bcal
