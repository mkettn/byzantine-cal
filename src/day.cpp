#include "day.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <string_view>
#include <vector>

namespace bcal {

namespace {

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

DayResult fail(std::string msg) { return {std::nullopt, std::move(msg)}; }

// Parses 1-2 digits from s; returns nullopt if not purely digits.
std::optional<int> small_int(std::string_view s) {
  if (s.empty() || s.size() > 2) return std::nullopt;
  int v = 0;
  for (char c : s) {
    if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
    v = v * 10 + (c - '0');
  }
  return v;
}

int days_in_month(int month) {
  static constexpr std::array<int, 12> d = {31, 29, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};
  return d[static_cast<size_t>(month - 1)];  // Feb: 29 (valid in leap years)
}

DayResult parse_solar(const std::string& t) {
  auto dot = t.find('.');
  if (dot == std::string::npos || t.find('.', dot + 1) != std::string::npos)
    return fail("expected DD.MM (e.g. \"25.12\"), got \"" + t + "\"");
  auto d = small_int(std::string_view(t).substr(0, dot));
  auto m = small_int(std::string_view(t).substr(dot + 1));
  if (!d || !m)
    return fail("day and month must be numbers (DD.MM), got \"" + t + "\"");
  if (*m < 1 || *m > 12)
    return fail("month must be 1-12 in \"" + t + "\"");
  if (*d < 1 || *d > days_in_month(*m))
    return fail("day " + std::to_string(*d) + " does not exist in month " +
                std::to_string(*m) + " (\"" + t + "\")");
  Day r;
  r.kind = Day::Kind::Solar;
  r.day = *d;
  r.month = *m;
  return {r, {}};
}

DayResult parse_pascha(const std::string& t) {
  Day r;
  r.kind = Day::Kind::Pascha;
  size_t i = 1;  // skip 'p'

  if (i < t.size() && (t[i] == '+' || t[i] == '-')) {
    int sign = t[i] == '-' ? -1 : 1;
    ++i;
    size_t start = i;
    while (i < t.size() && std::isdigit(static_cast<unsigned char>(t[i]))) ++i;
    if (start == i)
      return fail("missing number after sign in \"" + t + "\"");
    if (i - start > 4) return fail("number too large in \"" + t + "\"");
    int n = std::stoi(t.substr(start, i - start));
    if (i < t.size() && (t[i] == 'd' || t[i] == 'w')) {
      if (t[i] == 'w') n *= 7;
      ++i;
    }
    r.offset_days = sign * n;
  }

  if (i < t.size()) {
    char c = t[i];
    if (c != '<' && c != '>')
      return fail("unexpected '" + std::string(1, c) + "' in \"" + t +
                  "\" (expected d, w, < or >)");
    auto wd = parse_weekday(t.substr(i + 1));
    if (!wd)
      return fail("unknown weekday \"" + t.substr(i + 1) + "\" in \"" + t +
                  "\" (use mon..sun)");
    (c == '<' ? r.prev_weekday : r.next_weekday) = wd;
  }
  return {r, {}};
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

}  // namespace

std::optional<int> parse_weekday(const std::string& name) {
  static constexpr std::array<std::string_view, 7> names = {
      "mon", "tue", "wed", "thu", "fri", "sat", "sun"};
  auto n = lower(name);
  for (size_t i = 0; i < names.size(); ++i)
    if (n == names[i]) return static_cast<int>(i);
  return std::nullopt;
}

DayResult parse_day(const std::string& text) {
  if (text.empty()) return fail("empty date");
  if (text.find_first_of(" \t") != std::string::npos)
    return fail("whitespace not allowed in date \"" + text + "\"");
  return (text[0] == 'p' || text[0] == 'P') ? parse_pascha(lower(text))
                                            : parse_solar(text);
}

// Weekday key grammar: either one range "mon..fri" (wrapping allowed, e.g.
// "sat..mon") or a comma list of single days "wed,fri" / "sat". Lowercase only.
// Returns the weekdays (0 = mon .. 6 = sun) it covers; sets err on failure.
Weekdays parse_weekday_key(const std::string& key, std::string& err) {
  Weekdays days;
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

}  // namespace bcal
