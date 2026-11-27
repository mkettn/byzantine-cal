#include <cstdio>
#include <string>

#include "day.h"
#include "pascha.h"
#include "validator.h"
#include "calendar_loader.h"
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
  CHECK(is_pascha(2000, 4, 30));
  CHECK(is_pascha(2019, 4, 28));
  CHECK(is_pascha(2023, 4, 16));
  CHECK(is_pascha(2024, 5, 5));
  CHECK(is_pascha(2025, 4, 20));
  CHECK(is_pascha(2026, 4, 12));
  CHECK(is_pascha(2027, 5, 2));
  CHECK(is_pascha(2028, 4, 16));
  CHECK(is_pascha(2012, 4, 15));
  {
    bool lo = false, hi = false;
    try { bcal::orthodox_pascha(0); } catch (const std::out_of_range&) { lo = true; }
    try { bcal::orthodox_pascha(10000); } catch (const std::out_of_range&) { hi = true; }
    CHECK(lo && hi);
  }

  // --- YearCalendar: size, iteration, queries
  {
    auto c = cal("[]", 2028);                  // leap year
    CHECK(c.size() == 366 && c.year() == 2028);
    CHECK(cal("[]", 2027).size() == 365);
    CHECK(c.begin()->date == year{2028} / January / 1);
    CHECK((c.end() - 1)->date == year{2028} / December / 31);
    CHECK(c.at(2, 29).date == year{2028} / February / 29);
    CHECK(c.at(year{2028} / March / 1).weekday() == Wednesday);
    bool threw = false;
    try { c.at(year{2029} / January / 1); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { cal("[]", 2027).at(2, 29); } catch (const std::out_of_range&) { threw = true; }
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
    // Queries are Gregorian: the rule written for 25.12. is found on 7.1. with
    // an offset of 13, and at(12, 25) holds whatever is written for 12.12.
    CHECK(data_at(c13, 1, 7) == "Nativity");   // 25.12.2025 + 13
    CHECK(data_at(c13, 12, 25) == "-");
    CHECK(data_at(c13, 1, 6) == "-");
    int hits = 0;
    for (const auto& d : c13) hits += d.has_rule();
    CHECK(hits == 1);                          // 25.12.2026 + 13 is in 2027
    auto c12 = cal("- \"12.12\": Old\n- \"25.12\": New\n", 2026, 13);
    CHECK(data_at(c12, 12, 25) == "Old");       // 12.12. + 13 = 25.12.
    CHECK(data_at(c12, 1, 7) == "New");
    CHECK(data_at(cal("- \"12.12\": Old\n- \"25.12\": New\n", 2026), 12, 25) == "New");
    auto cm = cal(y, 2026, -13);
    CHECK(data_at(cm, 12, 12) == "Nativity");
    // negative offset pulls dates of the next year back
    auto cn = cal("- \"05.01\": X\n", 2026, -10);
    CHECK(data_at(cn, 12, 26) == "X" && data_at(cn, 1, 5) == "-");
    // offset also moves the fixed date across month ends
    auto cf = cal("- \"28.02\": X\n", 2026, 1);
    CHECK(data_at(cf, 3, 1) == "X");
  }

  // --- any year, also at the ends: fixed dates still leap in from outside
  {
    auto c = cal("- \"25.12\": Nativity\n- \"p\": P\n", 2012, 13);
    CHECK(data_at(c, 1, 7) == "Nativity" && data_at(c, 4, 15) == "P");
    auto first = cal("- \"25.12\": N\n", 1, 13);           // 25.12.0 + 13
    CHECK(data_at(first, 1, 7) == "N");
    auto e = cal("- \"05.01\": X\n", 9999, -10);          // 05.01.10000 - 10
    CHECK(data_at(e, 12, 26) == "X");
  }

  // --- leap day
  {
    const char* y = "- \"29.02\": Leap\n";
    auto c24 = cal(y, 2028);
    CHECK(data_at(c24, 2, 29) == "Leap");
    auto c25 = cal(y, 2027);                   // no 29.02. -> no day, no error
    int hits = 0;
    for (const auto& d : c25) hits += d.has_rule();
    CHECK(hits == 0);
    auto c24o = cal(y, 2028, 13);              // 29.02.2028 + 13 = 13.03.
    CHECK(data_at(c24o, 3, 13) == "Leap" && data_at(c24o, 2, 29) == "-");
    auto c25o = cal(y, 2027, 13);              // 29.02.2028 + 13 is not in 2027
    hits = 0;
    for (const auto& d : c25o) hits += d.has_rule();
    CHECK(hits == 0);
  }

  // --- Pascha-relative dates are not shifted by the offset
  {
    const char* y = "- \"p\": Pascha\n- \"p+39\": Ascension\n- \"p-7w<wed\": W\n";
    for (int off : {0, 13, -20}) {
      auto c = cal(y, 2028, off);
      CHECK(data_at(c, 4, 16) == "Pascha");
      CHECK(data_at(c, 5, 25) == "Ascension");
      CHECK(data_at(c, 2, 23) == "W");          // 27.2. is a Sunday; previous Wednesday
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

    // Pascha range
    auto p = cal("- \"p..p+6d\":\n    mon..sun: no_fast\n", 2028);
    CHECK(data_at(p, 4, 16) == "no_fast" && data_at(p, 4, 22) == "no_fast");
    CHECK(data_at(p, 4, 23) == "-" && data_at(p, 4, 15) == "-");

    // later entries / later weekday keys win
    auto l = cal("- \"01.03\": A\n- \"01.03\": B\n"
                 "- \"02.03..03.03\":\n    mon..sun: no_meat\n    mon: no_dairy\n"
                 "- \"03.03\": E\n", 2026);   // 2.3.2026 is a Monday
    CHECK(data_at(l, 3, 1) == "B");
    CHECK(data_at(l, 3, 2) == "no_dairy");
    CHECK(data_at(l, 3, 3) == "E");
  }

  // --- a range with a bound that does not exist is ignored as a whole
  {
    auto day = [](const char* s) { return *bcal::parse_day(s).day; };
    bcal::YearCalendar c(2027);                     // not a leap year
    c.add_range(day("25.02"), day("29.02"), "r");
    c.add_range(day("29.02"), day("02.03"), "s");
    int hits = 0;
    for (const auto& x : c) hits += x.has_rule();
    CHECK(hits == 0);
    bcal::YearCalendar l(2028);                     // leap year
    l.add_range(day("27.02"), day("01.03"), "r");
    CHECK(data_at(l, 2, 29) == "r" && data_at(l, 3, 1) == "r");
  }

  // --- Pascha-relative dates refer to the Pascha of this year only
  {
    auto c = cal("- \"p+100d\": in\n- \"p+300d\": out\n- \"p-300d\": out\n", 2026);
    CHECK(data_at(c, 7, 21) == "in");      // Pascha 2026 (12.4.) + 100 days
    int hits = 0;
    for (const auto& d : c) hits += d.has_rule();
    CHECK(hits == 1);                      // the others fall outside this year
  }

  // --- the API without YAML
  {
    bcal::YearCalendar c(2026, 13);
    CHECK(c.year() == 2026 && c.offset_days() == 13);
    CHECK(c.pascha() == year{2026} / April / 12);           // not shifted
    auto day = [](const char* s) { return *bcal::parse_day(s).day; };

    c.add_day(day("25.12"), "nativity");
    CHECK(data_at(c, 1, 7) == "nativity" && data_at(c, 12, 25) == "-");

    c.add_day(day("p"), "pascha");
    CHECK(data_at(c, 4, 12) == "pascha");

    // range with weekday filter (14.-20.3. after the shift: a Saturday is 14.3.)
    c.add_range(day("01.03"), day("07.03"), {5, 6}, "weekend");   // sat, sun
    CHECK(data_at(c, 3, 14) == "weekend" && data_at(c, 3, 15) == "weekend");
    CHECK(data_at(c, 3, 16) == "-" && data_at(c, 3, 13) == "-");

    // no filter: every day; later calls win
    c.add_range(day("01.03"), day("03.03"), "all");               // 14.-16.3.
    CHECK(data_at(c, 3, 14) == "all" && data_at(c, 3, 16) == "all");
    c.add_range(day("02.03"), day("02.03"), {}, "none");          // empty filter
    CHECK(data_at(c, 3, 15) == "all");
    c.add_day(day("02.03"), "later");
    CHECK(data_at(c, 3, 15) == "later");

    // Pascha-relative range; arbitrary data is kept verbatim
    c.add_range(day("p"), day("p+2d"), "line1: a\nline2: \"b\"");
    CHECK(data_at(c, 4, 14) == "line1: a\nline2: \"b\"");
    CHECK(data_at(c, 4, 15) == "-");

    // 29.02. in a non-leap year is ignored
    c.add_day(day("29.02"), "leap");
    CHECK(data_at(c, 3, 13) != "leap");

    // creation checks
    bool a = false, b = false;
    try { bcal::YearCalendar(0); } catch (const std::invalid_argument&) { a = true; }
    try { bcal::YearCalendar(10000); } catch (const std::invalid_argument&) { b = true; }
    CHECK(a && b);
    CHECK(bcal::YearCalendar(2028).pascha() == year{2028} / April / 16);
  }

  // --- at() rejects out-of-range arguments instead of truncating them
  {
    auto c = cal("[]", 2026);
    auto throws_at = [&](unsigned m, unsigned d) {
      try { c.at(m, d); } catch (const std::out_of_range&) { return true; }
      return false;
    };
    CHECK(throws_at(1, 257) && throws_at(257, 1) && throws_at(0, 0) &&
          throws_at(13, 1) && throws_at(2, 29) && throws_at(4, 31));
    CHECK(!throws_at(1, 1) && !throws_at(12, 31));
  }

  // --- invalid input / parameters
  {
    auto throws = [](const char* y, int yr, int off) {
      try { cal(y, yr, off); } catch (const std::invalid_argument&) { return true; }
      return false;
    };
    CHECK(throws("- \"31.04\": x\n", 2026, 0));
    CHECK(throws("[]", 0, 0));
    CHECK(throws("[]", 10000, 0));
    CHECK(!throws("[]", 1, 0) && !throws("[]", 9999, 0));
    CHECK(!throws("[]", 2012, 0) && !throws("[]", 2100, 0));
  }

  std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
  return failures ? 1 : 0;
}
