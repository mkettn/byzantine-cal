#include <cstdio>
#include <string>

#include "day.h"
#include "pascha.h"
#include "template_backend.h"
#include "calendar_loader.h"
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
  return bcal::load_year_calendar(YAML::Load(yaml), y, off);
}
static int hits(const bcal::YearCalendar& c) {
  int n = 0;
  for (const auto& d : c) n += d.has_rule();
  return n;
}
template <class E, class F>
static bool throws(F f) {
  try { f(); } catch (const E&) { return true; }
  return false;
}
static bcal::Day D(const char* s) { return *bcal::parse_day(s).day; }

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
  CHECK(!yaml_ok("- \"01.01..02.01\":\n    mon: {rule: no_meat, rule: bogus}\n"));
  CHECK(!yaml_ok("- \"25.12\": {name: X, rule: no_meat, rule: bogus}\n"));


  // --- Orthodox Pascha (Gregorian dates)
  const int pascha[][3] = {{2000, 4, 30}, {2012, 4, 15}, {2019, 4, 28}, {2023, 4, 16}, {2024, 5, 5},
                           {2025, 4, 20}, {2026, 4, 12}, {2027, 5, 2},  {2028, 4, 16}};
  for (const auto& p : pascha) CHECK(is_pascha(p[0], p[1], p[2]));
  CHECK(throws<std::out_of_range>([] { bcal::orthodox_pascha(0); }));
  CHECK(throws<std::out_of_range>([] { bcal::orthodox_pascha(10000); }));

  // --- YearCalendar: size, iteration, queries
  {
    auto c = cal("[]", 2028);                  // leap year
    CHECK(c.size() == 366 && c.year() == 2028 && cal("[]", 2027).size() == 365);
    CHECK(c.begin()->date == year{2028} / January / 1);
    CHECK((c.end() - 1)->date == year{2028} / December / 31);
    CHECK(c.at(2, 29).date == year{2028} / February / 29);
    CHECK(c.at(year{2028} / March / 1).weekday() == Wednesday && hits(c) == 0);
    // invalid values are rejected, not truncated (257 would wrap to 1)
    const unsigned bad[][2] = {{1, 257}, {257, 1}, {0, 0}, {13, 1}, {4, 31}};
    for (const auto& b : bad) CHECK(throws<std::out_of_range>([&] { c.at(b[0], b[1]); }));
    CHECK(throws<std::out_of_range>([&] { c.at(year{2029} / January / 1); }));
    CHECK(throws<std::out_of_range>([] { cal("[]", 2027).at(2, 29); }));
  }

  // --- data: the rule's YAML as a single line, arbitrary fields included
  {
    auto c = cal("- \"25.12\": {name: Nativity, rule: no_fish, note: \"x y\", tags: [a, b]}\n"
                 "- \"06.01\": Theophany\n"
                 "- \"01.02..04.02\":\n    mon..sun: no_meat\n    wed: {rule: no_oil, level: 2}\n", 2026);
    CHECK(data_at(c, 12, 25) == "{name: Nativity, rule: no_fish, note: x y, tags: [a, b]}");
    CHECK(data_at(c, 1, 6) == "Theophany");
    CHECK(data_at(c, 2, 1) == "no_meat");
    CHECK(data_at(c, 2, 4) == "{rule: no_oil, level: 2}");   // Wednesday: the later key wins
    CHECK(data_at(c, 2, 5) == "-" && data_at(c, 1, 31) == "-");
  }

  // --- offset: shifts the fixed dates, also across year boundaries
  {
    const char* y = "- \"12.12\": Old\n- \"25.12\": New\n";
    auto c0 = cal(y, 2026), c13 = cal(y, 2026, 13), cm = cal(y, 2026, -13);
    CHECK(data_at(c0, 12, 25) == "New" && data_at(c0, 1, 7) == "-");
    // Queries are Gregorian: with 13 the rule written for 12.12. is found on
    // 25.12. and the one written for 25.12. on 7.1. (25.12.2025 + 13)
    CHECK(data_at(c13, 12, 25) == "Old" && data_at(c13, 1, 7) == "New" && data_at(c13, 1, 6) == "-");
    CHECK(hits(c13) == 2);                     // 25.12.2026 + 13 is in 2027
    CHECK(data_at(cm, 11, 29) == "Old" && data_at(cm, 12, 12) == "New" && hits(cm) == 2);
    CHECK(data_at(cal("- \"05.01\": X\n", 2026, -10), 12, 26) == "X");   // next year's date
    CHECK(data_at(cal("- \"28.02\": X\n", 2026, 1), 3, 1) == "X");
    CHECK(data_at(cal("- \"25.12\": N\n", 1, 13), 1, 7) == "N");         // any year
    CHECK(data_at(cal("- \"05.01\": X\n", 9999, -10), 12, 26) == "X");
    CHECK(data_at(cal("- \"25.12\": N\n- \"p\": P\n", 2012, 13), 4, 15) == "P");
  }

  // --- leap day
  {
    const char* l = "- \"29.02\": Leap\n";
    CHECK(data_at(cal(l, 2028), 2, 29) == "Leap" && hits(cal(l, 2027)) == 0);
    auto shifted = cal(l, 2028, 13);           // 29.02.2028 + 13 = 13.03.
    CHECK(data_at(shifted, 3, 13) == "Leap" && data_at(shifted, 2, 29) == "-");
    CHECK(hits(cal(l, 2027, 13)) == 0);
  }

  // --- Pascha-relative dates: of this year only, not shifted by the offset
  {
    const char* y = "- \"p\": Pascha\n- \"p+39\": Ascension\n- \"p-7w<wed\": W\n"
                    "- \"p+300d\": out\n- \"p-300d\": out\n";
    for (int off : {0, 13, -20}) {
      auto c = cal(y, 2028, off);
      CHECK(data_at(c, 4, 16) == "Pascha" && data_at(c, 5, 25) == "Ascension");
      CHECK(data_at(c, 2, 23) == "W");          // 27.2. is a Sunday; previous Wednesday
      CHECK(hits(c) == 3);                     // the last two fall outside the year
    }
  }

  // --- ranges: weekdays, crossing New Year, offset, later wins
  {
    auto c = cal("- \"01.03..14.03\":\n    wed,fri: no_oil\n", 2026);
    CHECK(data_at(c, 3, 4) == "no_oil" && data_at(c, 3, 6) == "no_oil");
    CHECK(data_at(c, 3, 5) == "-" && data_at(c, 3, 15) == "-");

    auto w = cal("- \"15.12..05.01\":\n    mon..sun: no_meat\n", 2026);
    CHECK(data_at(w, 1, 1) == "no_meat" && data_at(w, 1, 5) == "no_meat");
    CHECK(data_at(w, 1, 6) == "-" && data_at(w, 12, 14) == "-");
    CHECK(data_at(w, 12, 15) == "no_meat" && data_at(w, 12, 31) == "no_meat");

    // 15.11-24.12 shifted by 13 days: 28.11-06.01
    auto f = cal("- \"15.11..24.12\":\n    mon..sun: no_fish\n", 2026, 13);
    CHECK(data_at(f, 1, 6) == "no_fish" && data_at(f, 1, 7) == "-");
    CHECK(data_at(f, 11, 27) == "-" && data_at(f, 11, 28) == "no_fish");

    // weekdays are those of the shifted date (14.-20.3.)
    auto s = cal("- \"01.03..07.03\":\n    sun: no_oil\n", 2026, 13);
    CHECK(data_at(s, 3, 15) == "no_oil" && data_at(s, 3, 8) == "-");

    auto p = cal("- \"p..p+6d\":\n    mon..sun: no_fast\n", 2028);
    CHECK(data_at(p, 4, 16) == "no_fast" && data_at(p, 4, 22) == "no_fast");
    CHECK(data_at(p, 4, 23) == "-" && data_at(p, 4, 15) == "-");

    // later entries / later weekday keys win (2.3.2026 is a Monday)
    auto l = cal("- \"01.03\": A\n- \"01.03\": B\n"
                 "- \"02.03..03.03\":\n    mon..sun: no_meat\n    mon: no_dairy\n"
                 "- \"03.03\": E\n", 2026);
    CHECK(data_at(l, 3, 1) == "B" && data_at(l, 3, 2) == "no_dairy" && data_at(l, 3, 3) == "E");
  }

  // --- the API without YAML
  {
    bcal::YearCalendar c(2026, 13);
    CHECK(c.year() == 2026 && c.offset_days() == 13 && c.pascha() == year{2026} / April / 12);
    c.add_day(D("25.12"), "nativity");
    c.add_day(D("p"), "pascha");
    CHECK(data_at(c, 1, 7) == "nativity" && data_at(c, 12, 25) == "-" && data_at(c, 4, 12) == "pascha");

    c.add_range(D("01.03"), D("07.03"), {5, 6}, "weekend");     // 14.-20.3.: sat, sun
    CHECK(data_at(c, 3, 14) == "weekend" && data_at(c, 3, 15) == "weekend");
    CHECK(data_at(c, 3, 13) == "-" && data_at(c, 3, 16) == "-");
    c.add_range(D("01.03"), D("03.03"), bcal::all_weekdays(), "all");   // later calls win
    c.add_range(D("02.03"), D("02.03"), {}, "none");                    // empty filter: nothing
    CHECK(data_at(c, 3, 14) == "all" && data_at(c, 3, 15) == "all");
    c.add_day(D("02.03"), "line1: a\nline2: \"b\"");                    // data is kept verbatim
    CHECK(data_at(c, 3, 15) == "line1: a\nline2: \"b\"");

    // a range with a bound that does not exist is ignored as a whole
    bcal::YearCalendar n(2027), l(2028);
    n.add_range(D("25.02"), D("29.02"), bcal::all_weekdays(), "r");
    n.add_range(D("29.02"), D("02.03"), bcal::all_weekdays(), "s");
    l.add_range(D("27.02"), D("01.03"), bcal::all_weekdays(), "r");
    CHECK(hits(n) == 0 && data_at(l, 2, 29) == "r" && data_at(l, 3, 1) == "r");
  }

  // --- template backend
  {
    bcal::YearCalendar c(2026, 13);
    c.add_day(D("25.12"), "Nativity");
    c.add_range(D("p"), D("p+1d"), bcal::all_weekdays(), "a|b");
    auto j = bcal::calendar_to_json(c);
    CHECK(j["year"] == 2026 && j["offset_days"] == 13 && j["pascha"] == "2026-04-12");
    CHECK(j["days"].size() == 365 && j["months"].size() == 12);
    CHECK(j["months"][0]["name"] == "January" && j["months"][0]["rule_count"] == 1);
    CHECK(j["months"][3]["rule_count"] == 2 && j["months"][1]["rule_count"] == 0);
    CHECK(j["months"][0]["days"].size() == 31 && j["months"][1]["days"].size() == 28);
    const auto& d = j["months"][0]["days"][6];          // 7 January 2026 (25.12.2025 + 13)
    CHECK(d["date"] == "2026-01-07" && d["day"] == 7 && d["month"] == 1 && d["year"] == 2026);
    CHECK(d["weekday"] == 3 && d["weekday_name"] == "Wednesday");
    CHECK(d["has_rule"] == true && d["data"] == "Nativity");
    CHECK(j["days"][0]["has_rule"] == false && j["days"][0]["data"] == "");

    auto render = [&](const char* t) { return bcal::render_template(c, t); };
    CHECK(render("{{ year }}/{{ pascha }}") == "2026/2026-04-12");
    CHECK(render("## {{ year }}") == "## 2026");                      // not an inja line statement
    CHECK(render("{% for d in days %}\n{% if d.has_rule %}\n{{ d.date }}={{ d.data }};\n"
                 "{% endif %}\n{% endfor %}\n") == "2026-01-07=Nativity;\n2026-04-12=a|b;\n2026-04-13=a|b;\n");
    CHECK(render("{{ replace(\"a|b|c\", \"|\", \"\\\\|\") }}") == "a\\|b\\|c");
    CHECK(throws<bcal::TemplateError>([&] { render("{% if %}"); }));
    CHECK(throws<bcal::TemplateError>([&] { render("{{ no_such_value }}"); }));
  }

  // --- invalid input / parameters
  CHECK(throws<std::invalid_argument>([] { cal("- \"31.04\": x\n", 2026); }));
  for (int bad : {0, 10000}) CHECK(throws<std::invalid_argument>([&] { cal("[]", bad); }));
  for (int ok : {1, 2012, 2100, 9999}) CHECK(!throws<std::invalid_argument>([&] { cal("[]", ok); }));

  std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
  return failures ? 1 : 0;
}
