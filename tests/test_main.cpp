#include <cstdio>
#include <string>

#include "day.h"
#include "pascha.h"
#include "validator.h"
#include "year_calendar.h"

static int failures = 0;
#define CHECK(c)                                              \
  do {                                                        \
    if (!(c)) {                                               \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
      ++failures;                                             \
    }                                                         \
  } while (0)

static bool day_ok(const std::string& s) { return bcal::parse_day(s).day.has_value(); }
static bool yaml_ok(const std::string& s) { return bcal::validate(YAML::Load(s)).ok(); }

using namespace std::chrono;

static bool is_pascha(int y, unsigned m, unsigned d) {
  return bcal::orthodox_pascha(y) == year{y} / month{m} / day{d};
}
static std::string data_at(const bcal::YearCalendar& c, unsigned m, unsigned d) {
  return c.at(m, d).data.value_or("-");
}
static bcal::YearCalendar cal(const std::string& yaml, int y, int off = 0) {
  return bcal::YearCalendar(YAML::Load(yaml), y, off);
}

int main() {
  for (auto s : {"25.12", "6.1", "06.01", "29.02", "31.12", "p", "p+39", "p-7d",
                 "p-7w", "p-7w<wed", "p+1w>sun", "P+3"})
    CHECK(day_ok(s));
  for (auto s : {"", "0612", "12/25", "32.01", "31.04", "30.02", "00.05", "1.13",
                 "1.2.3", "a.b", "p+", "p+3x", "p-7w<xyz", "p-7w<", " 1.1", "p+99999", "e+39"})
    CHECK(!day_ok(s));

  auto d = bcal::parse_day("p-7w<wed");
  CHECK(d.day && d.day->offset_days == -49 && d.day->prev_weekday == 2);

  CHECK(yaml_ok("- \"25.12\": Nativity\n- 06.01: Theophany\n"));
  CHECK(yaml_ok("- \"25.12\": {name: X, rule: no_fish}\n"));
  CHECK(yaml_ok("- \"01.02..14.02\":\n    mon..fri: no_meat\n    sat,sun: no_dairy\n"));
  CHECK(yaml_ok("- \"p-49d..p\":\n    mon..sun: no_oil\n"));
  CHECK(yaml_ok("- \"15.12..05.01\":\n    wed: no_oil\n"));  // wraps year end

  CHECK(!yaml_ok("foo: bar\n"));
  CHECK(!yaml_ok("- \"31.04\": Bad\n"));
  CHECK(!yaml_ok("- \"01.01..32.01\":\n    mon: no_oil\n"));
  CHECK(!yaml_ok("- \"01.01..02.01..03.01\":\n    mon: no_oil\n"));
  CHECK(!yaml_ok("- \"01.01..02.01\": no_oil\n"));
  CHECK(!yaml_ok("- \"01.01..02.01\":\n    funday: no_oil\n"));
  CHECK(!yaml_ok("- \"01.01..02.01\":\n    mon: no_pizza\n"));
  CHECK(!yaml_ok("- \"25.12\": {rule: no_fish}\n"));
  CHECK(yaml_ok("- \"25.12\": {name: X, rule: no_fish, anything: [1, {a: b}]}\n"));
  CHECK(yaml_ok("- \"01.01..02.01\":\n    mon: {rule: no_oil, note: x}\n"));
  CHECK(!yaml_ok("- \"01.01..02.01\":\n    mon: {note: x}\n"));
  CHECK(!yaml_ok("- \"01.01..02.01\":\n    mon: {rule: bogus}\n"));


  // --- Orthodox Pascha (Gregorian dates)
  CHECK(is_pascha(2000, 4, 30));
  CHECK(is_pascha(2019, 4, 28));
  CHECK(is_pascha(2023, 4, 16));
  CHECK(is_pascha(2024, 5, 5));
  CHECK(is_pascha(2025, 4, 20));
  CHECK(is_pascha(2026, 4, 12));
  CHECK(is_pascha(2027, 5, 2));
  {
    bool lo = false, hi = false;
    try { bcal::orthodox_pascha(1899); } catch (const std::out_of_range&) { lo = true; }
    try { bcal::orthodox_pascha(2100); } catch (const std::out_of_range&) { hi = true; }
    CHECK(lo && hi);
  }

  // --- YearCalendar: size, iteration, queries
  {
    auto c = cal("[]", 2024);
    CHECK(c.size() == 366 && c.year() == 2024);
    CHECK(cal("[]", 2025).size() == 365);
    CHECK(c.begin()->date == year{2024} / January / 1);
    CHECK((c.end() - 1)->date == year{2024} / December / 31);
    CHECK(c.at(2, 29).date == year{2024} / February / 29);
    CHECK(c.at(year{2024} / March / 1).weekday() == Friday);
    bool threw = false;
    try { c.at(year{2025} / January / 1); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { cal("[]", 2025).at(2, 29); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    int n = 0, with_rule = 0;
    for (const auto& d : c) { ++n; with_rule += d.has_rule(); }
    CHECK(n == 366 && with_rule == 0);
  }

  // --- data is the rule's YAML as a single line
  {
    auto c = cal("- \"25.12\": Nativity\n"
                 "- \"06.01\": {name: Theophany, rule: no_fish}\n"
                 "- \"01.02..03.02\":\n    mon..sun: no_meat\n", 2026);
    CHECK(data_at(c, 12, 25) == "Nativity");
    CHECK(data_at(c, 1, 6) == "{name: Theophany, rule: no_fish}");
    CHECK(data_at(c, 2, 1) == "no_meat" && data_at(c, 2, 3) == "no_meat");
    CHECK(data_at(c, 2, 4) == "-" && data_at(c, 1, 31) == "-");
  }

  // --- arbitrary data next to the rule
  {
    auto c = cal("- \"25.12\": {name: Nativity, rule: no_fish, note: \"x y\", tags: [a, b]}\n"
                 "- \"01.03..02.03\":\n    mon..sun: {rule: no_oil, level: 2}\n", 2026);
    CHECK(data_at(c, 12, 25) == "{name: Nativity, rule: no_fish, note: x y, tags: [a, b]}");
    CHECK(data_at(c, 3, 1) == "{rule: no_oil, level: 2}");
  }

  // --- offset: shifts fixed dates and leaps over New Year
  {
    const char* y = "- \"25.12\": Nativity\n";
    auto c0 = cal(y, 2026);
    CHECK(data_at(c0, 12, 25) == "Nativity" && data_at(c0, 1, 7) == "-");
    auto c13 = cal(y, 2026, 13);
    CHECK(data_at(c13, 1, 7) == "Nativity");   // 25.12.2025 + 13
    CHECK(data_at(c13, 12, 25) == "-");
    CHECK(data_at(c13, 1, 6) == "-");
    int hits = 0;
    for (const auto& d : c13) hits += d.has_rule();
    CHECK(hits == 1);                          // 25.12.2026 + 13 is in 2027
    auto cm = cal(y, 2026, -13);
    CHECK(data_at(cm, 12, 12) == "Nativity");
    // negative offset pulls dates of the next year back
    auto cn = cal("- \"05.01\": X\n", 2026, -10);
    CHECK(data_at(cn, 12, 26) == "X" && data_at(cn, 1, 5) == "-");
    // offset also moves the fixed date across month ends
    auto cf = cal("- \"28.02\": X\n", 2026, 1);
    CHECK(data_at(cf, 3, 1) == "X");
  }

  // --- first / last supported year: fixed dates still leap in from outside
  {
    auto c = cal("- \"25.12\": Nativity\n- \"p\": P\n", 1900, 13);
    CHECK(data_at(c, 1, 7) == "Nativity");
    auto e = cal("- \"05.01\": X\n", 2099, -10);   // 05.01.2100 - 10
    CHECK(data_at(e, 12, 26) == "X");
  }

  // --- leap day
  {
    const char* y = "- \"29.02\": Leap\n";
    auto c24 = cal(y, 2024);
    CHECK(data_at(c24, 2, 29) == "Leap");
    auto c25 = cal(y, 2025);                   // no 29.02. -> no day, no error
    int hits = 0;
    for (const auto& d : c25) hits += d.has_rule();
    CHECK(hits == 0);
    auto c24o = cal(y, 2024, 13);              // 29.02.2024 + 13 = 13.03.
    CHECK(data_at(c24o, 3, 13) == "Leap" && data_at(c24o, 2, 29) == "-");
    auto c25o = cal(y, 2025, 13);              // 29.02.2024 + 13 is not in 2025
    hits = 0;
    for (const auto& d : c25o) hits += d.has_rule();
    CHECK(hits == 0);
    // 29.02.2028 + 365 = 28.02.2029 is not leap-related but shows leaping
    auto c29 = cal("- \"29.02\": L\n", 2029, 365);
    CHECK(data_at(c29, 2, 28) == "L");
  }

  // --- Pascha-relative dates are not shifted by the offset
  {
    const char* y = "- \"p\": Pascha\n- \"p+39\": Ascension\n- \"p-7w<wed\": W\n";
    for (int off : {0, 13, -20}) {
      auto c = cal(y, 2024, off);
      CHECK(data_at(c, 5, 5) == "Pascha");
      CHECK(data_at(c, 6, 13) == "Ascension");
      CHECK(data_at(c, 3, 13) == "W");          // 17.3. is a Sunday; previous Wednesday
    }
  }

  // --- ranges: weekdays, wrap over New Year, offset, later wins
  {
    auto c = cal("- \"01.03..14.03\":\n    wed,fri: no_oil\n", 2026);
    CHECK(data_at(c, 3, 4) == "no_oil" && data_at(c, 3, 6) == "no_oil");
    CHECK(data_at(c, 3, 5) == "-" && data_at(c, 3, 15) == "-");

    auto w = cal("- \"15.12..05.01\":\n    mon..sun: no_meat\n", 2026);
    CHECK(data_at(w, 1, 1) == "no_meat" && data_at(w, 1, 5) == "no_meat");
    CHECK(data_at(w, 1, 6) == "-" && data_at(w, 12, 14) == "-");
    CHECK(data_at(w, 12, 15) == "no_meat" && data_at(w, 12, 31) == "no_meat");

    // Nativity fast shifted by 13 days: 15.11-24.12 -> 28.11-06.01
    auto f = cal("- \"15.11..24.12\":\n    mon..sun: no_fish\n", 2026, 13);
    CHECK(data_at(f, 1, 6) == "no_fish" && data_at(f, 1, 7) == "-");
    CHECK(data_at(f, 11, 27) == "-" && data_at(f, 11, 28) == "no_fish");

    // weekdays are those of the shifted date
    auto s = cal("- \"01.03..07.03\":\n    sun: no_oil\n", 2026, 13);  // 14.-20.03.
    CHECK(data_at(s, 3, 15) == "no_oil" && data_at(s, 3, 8) == "-");

    // Pascha range, including one that reaches from last year into this one
    auto p = cal("- \"p..p+6d\":\n    mon..sun: no_fast\n", 2024);
    CHECK(data_at(p, 5, 5) == "no_fast" && data_at(p, 5, 11) == "no_fast");
    CHECK(data_at(p, 5, 12) == "-");

    // later entries / later weekday keys win
    auto l = cal("- \"01.03\": A\n- \"01.03\": B\n"
                 "- \"02.03..03.03\":\n    mon..sun: no_meat\n    mon: no_dairy\n"
                 "- \"03.03\": E\n", 2026);   // 2.3.2026 is a Monday
    CHECK(data_at(l, 3, 1) == "B");
    CHECK(data_at(l, 3, 2) == "no_dairy");
    CHECK(data_at(l, 3, 3) == "E");
  }

  // --- invalid input / parameters
  {
    auto throws = [](const char* y, int yr, int off) {
      try { cal(y, yr, off); } catch (const std::invalid_argument&) { return true; }
      return false;
    };
    CHECK(throws("- \"31.04\": x\n", 2026, 0));
    CHECK(throws("[]", 1899, 0));
    CHECK(throws("[]", 2100, 0));
    CHECK(!throws("[]", 1900, 0) && !throws("[]", 2099, 0));
    CHECK(throws("[]", 2026, 366));
    CHECK(throws("[]", 2026, -366));
    CHECK(!throws("[]", 2026, 365));
  }

  std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
  return failures ? 1 : 0;
}
