#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "day.h"
#include "pascha.h"
#include "template_backend.h"
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include "byzantine_cal_config.h"
#include "calendar_loader.h"
#include "year_calendar.h"
#include "entry.h"

static int failures = 0;
#define CHECK(c)                                              \
  do {                                                        \
    if (!(c)) {                                               \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
      ++failures;                                             \
    }                                                         \
  } while (0)

static bool day_ok(const std::string& s) { return bcal::parse_day(s).day.has_value(); }
static bool yaml_ok(const std::string& s) {
  try {
    bcal::load_year_calendar(YAML::Load(s), 2000);
    return true;
  } catch (const std::invalid_argument&) {
    return false;
  }
}

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
// A directory of its own that is removed, and the working directory restored, also if a check throws.
struct Sandbox {
  std::filesystem::path dir = std::filesystem::temp_directory_path() /
      ("bcal-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::path cwd = std::filesystem::current_path();
  Sandbox() { std::filesystem::create_directories(dir); }
  ~Sandbox() {
    std::error_code ec;
    std::filesystem::current_path(cwd, ec);
    std::filesystem::remove_all(dir, ec);
  }
  std::filesystem::path write(const char* name, const std::string& text) const {
    std::filesystem::create_directories((dir / name).parent_path());
    std::ofstream(dir / name) << text;
    return dir / name;
  }
};
static bcal::Entry load_one(const std::filesystem::path& file) { return bcal::load_entry_file(file).at(0); }
static bcal::Day D(const char* s) { return *bcal::parse_day(s).day; }
// A language from the language files of the sources.
static bcal::Language lang(const char* name = "en_US") {
  static const bcal::ByzantineCalConfig config(BCAL_TEST_CONFIG);
  return bcal::Language(config.find_language_file(name),
                        [&](const std::string& l) { return config.find_language_file(l); });
}
// For language files without a fallback.
static const bcal::LanguageFileFinder no_fallback = [](const std::string& l) -> std::filesystem::path {
  throw bcal::ConfigFileError("no language file for " + l);
};

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
  {
    std::string msg;
    try { bcal::load_year_calendar(YAML::Load("- \"01.01\": A\n- \"31.04\": B\n"), 2026, 0, "cal.yml"); }
    catch (const std::invalid_argument& e) { msg = e.what(); }
    CHECK(msg.find("cal.yml:2: invalid date") == 0);
    try { bcal::load_year_calendar(YAML::Load("- \"31.04\": B\n"), 2026); }
    catch (const std::invalid_argument& e) { msg = e.what(); }
    CHECK(msg.find("line 1: invalid date") == 0);
  }
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
    auto j = bcal::calendar_to_json(c, lang());
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

    // the names and texts of the language
    auto jd = bcal::calendar_to_json(c, lang("de_AT"));
    CHECK(jd["months"][0]["name"] == "Jänner" && jd["months"][1]["name"] == "Februar");
    CHECK(jd["months"][0]["days"][6]["weekday_name"] == "Mittwoch" && jd["ui"]["date"] == "Datum");
    CHECK(j["ui"]["date"] == "Date" && j["ui"]["pascha"] == "Pascha");
    auto render = [&](const char* t) { return bcal::render_template(c, lang(), t); };
    CHECK(render("{{ year }}/{{ pascha }}") == "2026/2026-04-12");
    CHECK(render("## {{ year }}") == "## 2026");                      // not an inja line statement
    CHECK(render("{% for d in days %}\n{% if d.has_rule %}\n{{ d.date }}={{ d.data }};\n"
                 "{% endif %}\n{% endfor %}\n") == "2026-01-07=Nativity;\n2026-04-12=a|b;\n2026-04-13=a|b;\n");
    CHECK(bcal::escape_markdown("a|b `c` *d* _e_ [f](g) <h> #i +j -k l. m! ~n &o \\p") ==
          "a\\|b \\`c\\` \\*d\\* \\_e\\_ \\[f\\]\\(g\\) \\<h\\> \\#i \\+j \\-k l\\. m\\! \\~n \\&o \\\\p");
    CHECK(bcal::escape_markdown("plain text 123") == "plain text 123");
    CHECK(bcal::escape_markdown("a\nb\r\nc") == "a<br>b<br>c" && bcal::escape_markdown("") == "");
    CHECK(render("{{ escape_markdown(\"a|b_c\") }}") == "a\\|b\\_c");
    CHECK(throws<bcal::TemplateError>([&] { render("{{ escape_markdown(1) }}"); }));
    CHECK(throws<bcal::TemplateError>([&] { render("{% if %}"); }));
    CHECK(throws<bcal::TemplateError>([&] { render("{{ no_such_value }}"); }));
    // any failure caused by a template, e.g. a list that cannot be allocated
    CHECK(throws<bcal::TemplateError>([&] { render("{{ range(99999999999999) }}"); }));
    CHECK(throws<bcal::TemplateError>([&] { render("{{ 1 / 0 }}"); }));
    CHECK(throws<bcal::TemplateError>([&] { render("{{ escape_markdown() }}"); }));
    CHECK(throws<bcal::TemplateError>([&] { render("{{ upper(1) }}"); }));
  }

  // --- backends read their template from a file
  {
    bcal::YearCalendar c(2026, 13);
    c.add_day(D("25.12"), "Nativity|x");
    auto test_tpl = [](const char* name) { return std::string(BCAL_TEST_TPL_DIR) + "/" + name; };
    CHECK(bcal::TemplateBackend(test_tpl("list.txt.inja")).render(c, lang()) == "2026-01-07;3;Nativity|x\n");
    const auto md = bcal::MarkdownBackend(std::string(BCAL_TPL_DIR) + "/" +
                                          bcal::MarkdownBackend::kTemplateFile).render(c, lang());
    CHECK(md.find("# 2026\n\nPascha: 2026-04-12\n\n## January\n") == 0);
    CHECK(md.find("| 2026-01-07 | Wednesday | Nativity\\|x |\n") != std::string::npos);
    CHECK(md.find("## February") == std::string::npos);            // no rules: no table
    CHECK(throws<bcal::TemplateFileError>([&] { bcal::TemplateBackend(test_tpl("nope.inja")); }));
    CHECK(throws<bcal::TemplateFileError>([&] { bcal::TemplateBackend(BCAL_TEST_TPL_DIR); }));   // a directory
    CHECK(throws<bcal::TemplateFileError>([] { bcal::MarkdownBackend("/no/such/file.inja"); }));
  }

  {
    CHECK(bcal::escape_html("a<b>&\"c\" 'd'\r\ne") == "a&lt;b&gt;&amp;&quot;c&quot; &#39;d&#39;<br>e" && bcal::escape_html("") == "");
    bcal::YearCalendar c(2026, 13);
    c.add_day(D("25.12"), "Nativity <b>&");
    const auto html = bcal::HtmlBackend(std::string(BCAL_TPL_DIR) + "/" + bcal::HtmlBackend::kTemplateFile)
                          .render(c, lang());
    CHECK(html.find("<!DOCTYPE html>\n<html>\n") == 0 && html.find("<h1>2026</h1>\n<p>Pascha: 2026-04-12</p>\n<h2>January</h2>\n") != std::string::npos);
    CHECK(html.find("<tr><td>2026-01-07</td><td>Wednesday</td><td>Nativity &lt;b&gt;&amp;</td></tr>\n") != std::string::npos);
    CHECK(html.find("<h2>February") == std::string::npos && html.find("<b>") == std::string::npos);
    CHECK(html.size() > 7 && html.compare(html.size() - 8, 8, "</html>\n") == 0);
  }

  {
    CHECK(bcal::escape_ics("a,b;c\\d\r\ne") == "a\\,b\\;c\\\\d\\ne" && bcal::escape_ics("") == "");
    CHECK(bcal::fold_ics("a\nb\n") == "a\r\nb\r\n" && bcal::fold_ics("") == "");
    const auto folded = bcal::fold_ics(std::string(80, 'x') + "\n");
    CHECK(folded == std::string(75, 'x') + "\r\n " + std::string(5, 'x') + "\r\n");
    CHECK(bcal::fold_ics(std::string(75, 'x') + "\n") == std::string(75, 'x') + "\r\n");   // fits
    CHECK(bcal::fold_ics(std::string(76, 'x')) == std::string(75, 'x') + "\r\n x\r\n");
    CHECK(bcal::fold_ics(std::string(75 + 74 + 74, 'x') + "\n") ==
          std::string(75, 'x') + "\r\n " + std::string(74, 'x') + "\r\n " + std::string(74, 'x') + "\r\n");
    CHECK(bcal::fold_ics("a\r\nb\r\n") == "a\r\nb\r\n");
    CHECK(bcal::fold_ics(std::string(100, '\x80') + "\n").size() > 100);   // invalid UTF-8 ends
    // 74 octets, then a 2-octet character that would end at octet 76: it moves to the next line
    CHECK(bcal::fold_ics(std::string(74, 'x') + "\xC3\xA4\n") == std::string(74, 'x') + "\r\n \xC3\xA4\r\n");
    bcal::YearCalendar c(2026, 13);
    c.add_day(D("25.12"), "Nativity, a;b");
    const auto ics = bcal::IcsBackend(std::string(BCAL_TPL_DIR) + "/" + bcal::IcsBackend::kTemplateFile)
                         .render(c, lang());
    CHECK(ics.find("BEGIN:VCALENDAR\r\nVERSION:2.0\r\n") == 0);
    CHECK(ics.find("BEGIN:VEVENT\r\nUID:2026-01-07@byzantine-cal\r\nDTSTAMP:20260107T000000Z\r\n"
                   "DTSTART;VALUE=DATE:20260107\r\nSUMMARY:Nativity\\, a\\;b\r\nEND:VEVENT\r\n") != std::string::npos);
    CHECK(ics.rfind("END:VCALENDAR\r\n") == ics.size() - 15);
  }

  // --- configuration
  {
    namespace fs = std::filesystem;
    Sandbox sandbox;
    const fs::path& dir = sandbox.dir;
    fs::create_directories(dir / "a" / "tpl" / "dirname");
    fs::create_directories(dir / "b");
    auto write = [&](const char* name, const std::string& text) {
      std::ofstream(dir / name) << text;
      return dir / name;
    };
    write("a/tpl/one.inja", "1");
    write("b/one.inja", "other");
    write("b/two.inja", "2");

    // template_dir: absolute names stay, relative ones are relative to the config file
    const auto file = write("a/bcal.yaml", "template_dir:\n  - tpl\n  - ../b\n  - /abs/dir\n");
    const bcal::ByzantineCalConfig c(file);
    CHECK(c.file() == file);
    CHECK((c.template_dirs() == std::vector<fs::path>{dir / "a" / "tpl", dir / "b", "/abs/dir"}));
    CHECK(c.find_template("one.inja") == dir / "a" / "tpl" / "one.inja");   // the file, not the directory
    CHECK(c.find_template("two.inja") == dir / "b" / "two.inja");
    CHECK(throws<bcal::ConfigFileError>([&] { c.find_template("none.inja"); }));
    CHECK(throws<bcal::ConfigFileError>([&] { c.find_template("dirname"); }));   // a directory is no template
    // the path can be given to a backend as it is
    write("a/tpl/markdown.inja", "year {{ year }}");
    CHECK(bcal::MarkdownBackend(c.find_template(bcal::MarkdownBackend::kTemplateFile))
              .render(bcal::YearCalendar(2026), lang()) == "year 2026");
    try {
      c.find_template("none.inja");
    } catch (const bcal::ConfigFileError& e) {
      const std::string what = e.what();
      CHECK(what.find("cannot find the template none.inja in the template_dir of ") == 0);
      CHECK(what.find((dir / "b").string()) != std::string::npos);        // the directories it looked in
    }
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::ByzantineCalConfig(write("empty.yaml", "")).find_template("x"); }));
    CHECK(bcal::ByzantineCalConfig(write("empty.yaml", "")).template_dirs().empty());
    CHECK(bcal::ByzantineCalConfig(write("none.yaml", "template_dir: []\n")).template_dirs().empty());

    // 0 = fine, 1 = invalid content, 2 = file cannot be read
    auto kind = [&](const char* text) {
      try { bcal::ByzantineCalConfig(write("x.yaml", text)); }
      catch (const bcal::ConfigFileError&) { return 2; }
      catch (const bcal::ConfigError&) { return 1; }
      return 0;
    };
    CHECK(kind("template_dir: [a]\n") == 0);
    for (const char* bad : {"- a\n", "template_dir: tpl\n", "template_dir:\n", "template_dir: [[a]]\n",
                            "template_dir: ['']\n", "template_dirs: [a]\n", "template_dir: [a]\nx: 1\n",
                            "template_dir: [a\n", "template_dir: [a]\ntemplate_dir: [b]\n",
                            "template_dir: [a]\n---\ntemplate_dir: [b]\n", "lang_dir: lang\n", "lang_dir: [[a]]\n",
                            "lang_dir: [a]\nlang_dir: [b]\n", "language: [de]\n", "language:\n", "language: de_DE\nlanguage: en\n"})
      CHECK(kind(bad) == 1);
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::ByzantineCalConfig(dir / "nope.yaml"); }));
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::ByzantineCalConfig{dir}; }));   // a directory
    try {
      bcal::ByzantineCalConfig(write("x.yaml", "template_dir: [a]\ntemplate_dirs: [b]\n"));
    } catch (const bcal::ConfigError& e) {
      const std::string what = e.what();
      CHECK(what.find("x.yaml:2: unknown option \"template_dirs\"") != std::string::npos);
    }

    // lang_dir and language: <language>.yaml in the first lang_dir that has it, any name
    fs::create_directories(dir / "a" / "lang");
    fs::create_directories(dir / "b" / "lang");
    write("a/lang/de_DE.yaml", "");
    write("b/lang/de_DE.yaml", "");
    write("b/lang/foo.bar.yaml", "");
    const bcal::ByzantineCalConfig lc(write("a/l.yaml", "lang_dir: [lang, ../b/lang]\nlanguage: de_AT\n"));
    CHECK(lc.language() == "de_AT" && !c.language());
    CHECK(lc.find_language_file("de_DE") == dir / "a/lang/de_DE.yaml");
    CHECK(lc.find_language_file("foo.bar") == dir / "b/lang/foo.bar.yaml");   // no check of the name
    CHECK(throws<bcal::ConfigFileError>([&] { lc.find_language_file("de"); }));   // only <name>.yaml
    CHECK(throws<bcal::ConfigFileError>([&] { c.find_language_file("de_DE"); }));        // no lang_dir
    CHECK(bcal::ByzantineCalConfig(write("o.yaml", "language: any thing!\n")).language() == "any thing!");

    // which file: -C, else bcal.yaml in the working directory, else the installed one
    const std::string installed = (dir / "a" / "bcal.yaml").string();
    fs::current_path(dir / "a");                                              // has a bcal.yaml
    CHECK(bcal::ByzantineCalConfig::find_file(std::nullopt, "/no/such/file") == "bcal.yaml");
    CHECK(bcal::ByzantineCalConfig::find_file(std::string("given.yaml"), installed) == "given.yaml");
    fs::current_path(dir / "b");                                              // has none
    CHECK(bcal::ByzantineCalConfig::find_file(std::nullopt, installed) == installed);
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::ByzantineCalConfig::find_file(std::nullopt, "/no/such/file"); }));
    // something unusable named bcal.yaml is not skipped, it is reported when it is read
    fs::create_directories(dir / "c" / "bcal.yaml");                          // a directory
    fs::create_directories(dir / "d");
    fs::create_symlink("bcal.yaml", dir / "d" / "bcal.yaml");                 // a symlink loop
    fs::create_directories(dir / "e");
    fs::create_symlink("nowhere.yaml", dir / "e" / "bcal.yaml");              // a dangling symlink
    for (const char* sub : {"c", "d", "e"}) {
      fs::current_path(dir / sub);
      const auto found = bcal::ByzantineCalConfig::find_file(std::nullopt, installed);
      CHECK(found == "bcal.yaml");
      CHECK(throws<bcal::ConfigFileError>([&] { bcal::ByzantineCalConfig{found}; }));
    }
  }

  // --- languages
  {
    Sandbox sb;
    const auto en = lang("en_US"), de = lang("de_DE"), at = lang("de_AT");
    CHECK(en.month(1) == "January" && en.month(12) == "December" && en.weekday(7) == "Sunday" && en.ui()["pascha"] == "Pascha");
    CHECK(de.month(3) == "März" && de.weekday(3) == "Mittwoch" && de.ui()["date"] == "Datum" && de.month(1) == "Januar");
    CHECK(at.month(1) == "Jänner" && at.month(2) == "Februar" && at.weekday(1) == "Montag" && at.ui()["weekday"] == "Wochentag");
    CHECK(throws<std::out_of_range>([&] { en.month(0); }) && throws<std::out_of_range>([&] { en.weekday(8); }));

    // fallbacks: de_CH -> de_DE -> en_US; the fallback is read first, so the keys of a file
    // replace those of its fallback (ui entry by entry)
    const std::string all = "months: [a, b, c, d, e, f, g, h, i, j, k, l]\nweekdays: [a, b, c, d, e, f, g]\n";
    sb.write("fb/cfg.yaml", "lang_dir: [.]\n");
    sb.write("fb/en_US.yaml", all + "ui: {date: EN, weekday: EN, data: EN, pascha: EN}\n");
    sb.write("fb/de_DE.yaml", "fallback: en_US\nmonths: [J, f, m, a, m, j, j, a, s, o, n, d]\nui: {date: DE, weekday: DE}\n");
    sb.write("fb/de_CH.yaml", "fallback: de_DE\nui: {date: CH}\n");
    const bcal::ByzantineCalConfig fb(sb.dir / "fb/cfg.yaml");
    const bcal::LanguageFileFinder find = [&](const std::string& l) { return fb.find_language_file(l); };
    auto log_of = [](auto f) {   // what is logged while f runs
      std::ostringstream log;
      const auto old = spdlog::default_logger();
      spdlog::set_default_logger(std::make_shared<spdlog::logger>("t", std::make_shared<spdlog::sinks::ostream_sink_mt>(log)));
      f();
      spdlog::set_default_logger(old);
      return log.str();
    };
    CHECK(log_of([&] {
      const bcal::Language ch(find("de_CH"), find);
      CHECK(ch.ui()["date"] == "CH" && ch.ui()["weekday"] == "DE" && ch.ui()["data"] == "EN");
      CHECK(ch.month(1) == "J" && ch.weekday(1) == "a");   // months from de_DE, weekdays from en_US
    }).empty());

    // every file is read only once: a circular fallback is ignored with a warning
    sb.write("fb/aa.yaml", "fallback: bb\nui: {date: A}\n");
    sb.write("fb/bb.yaml", "fallback: aa\n" + all + "ui: {date: B, weekday: B}\n");
    sb.write("fb/ss.yaml", "fallback: ss\n" + all);   // its own fallback
    const auto warnings = [](const std::string& log) {
      std::size_t n = 0;
      for (auto at = log.find("already read"); at != std::string::npos; at = log.find("already read", at + 1)) ++n;
      return n;
    };
    CHECK(warnings(log_of([&] {
      const bcal::Language a(find("aa"), find);
      CHECK(a.ui()["date"] == "A" && a.ui()["weekday"] == "B" && a.month(2) == "b");
    })) == 1);
    CHECK(warnings(log_of([&] { bcal::Language(find("ss"), find); })) == 1);

    // errors of fallbacks
    auto fails = [&](const std::string& text, int expected) {   // 1 = invalid, 2 = a file is missing
      try { bcal::Language(sb.write("fb/x.yaml", text), find); }
      catch (const bcal::ConfigFileError&) { return expected == 2; }
      catch (const bcal::ConfigError&) { return expected == 1; }
      return false;
    };
    CHECK(fails("fallback: zz_ZZ\n", 2));                  // no file
    CHECK(fails("fallback: [de]\n", 1) && fails("fallback:\n", 1));
    CHECK(fails("fallback: yy\n", 2));
    sb.write("fb/yy.yaml", "ui: {date: Y}\n");
    CHECK(fails("fallback: yy\n", 1));                      // no months in the end
    sb.write("fb/yy.yaml", "months: [a]\n");
    CHECK(fails("fallback: yy\n" + all, 1));                // the fallback file is invalid

    const std::string twelve = "[a, b, c, d, e, f, g, h, i, j, k, l]", seven = "[a, b, c, d, e, f, g]";
    auto kind = [&](const std::string& text) {   // 0 = fine, 1 = invalid, 2 = cannot be read
      try { bcal::Language(sb.write("x.yaml", text), no_fallback); }
      catch (const bcal::ConfigFileError&) { return 2; }
      catch (const bcal::ConfigError&) { return 1; }
      return 0;
    };
    CHECK(kind("months: " + twelve + "\nweekdays: " + seven + "\n") == 0);
    CHECK(kind("months: " + twelve + "\nweekdays: " + seven + "\nui: {a: b}\n") == 0);
    for (const std::string& bad : std::vector<std::string>{
             "months: [a, b]\nweekdays: " + seven + "\n",                              // 2 months
             "months: [a, b, c, d, e, f, g, h, i, j, k, [l]]\nweekdays: " + seven + "\n",
             "months: [a, b, c, d, e, f, g, h, i, j, k, '']\nweekdays: " + seven + "\n",
             "months: [a, b, c, d, e, f, g, h, i, j, k, ' ']\nweekdays: " + seven + "\n",   // blank name
             "months: " + twelve + "\nweekdays: " + seven + "\nui: [a]\n",
             "months: " + twelve + "\nweekdays: " + seven + "\ncolours: []\n",
             "- a\n", "months: [a\n", "",                                                // not a map, syntax, nothing
             "months: " + twelve + "\n",                                                  // no weekdays
             "weekdays: " + seven + "\n",                                                 // no months
             "months: " + twelve + "\nweekdays: " + seven + "\n---\nui: {}\n"})        // two documents
      CHECK(kind(bad) == 1);
    try {
      bcal::Language(sb.write("y.yaml", "months: [a, b]\n"), no_fallback);
    } catch (const bcal::ConfigError& e) {
      CHECK(std::string(e.what()).find("y.yaml:1: months must be a list of 12 names") != std::string::npos);
    }
    // the Markdown template escapes the texts of a language
    bcal::YearCalendar yc(2026, 13);
    yc.add_day(D("25.12"), "x");
    const auto md = bcal::MarkdownBackend(std::string(BCAL_TPL_DIR) + "/markdown.inja")
                        .render(yc, bcal::Language(sb.write("xx.yaml",
                            "months: ['Jan|x', b, c, d, e, f, g, h, i, j, k, l]\nweekdays: [a, b, 'W|3', d, e, f, g]\n"
                            "ui: {date: 'Da|te', weekday: w, data: d, pascha: '# P'}\n"), no_fallback));
    CHECK(md.find("## Jan\\|x") != std::string::npos && md.find("| Da\\|te |") != std::string::npos);
    CHECK(md.find("\\# P: 2026-04-12") != std::string::npos && md.find("| 2026-01-07 | W\\|3 |") != std::string::npos);
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::Language(sb.dir / "nope.yaml", no_fallback); }));
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::Language(sb.dir, no_fallback); }));   // a directory
  }

  {
    Sandbox sb;
    const auto file = sb.write("n.md",
        "---\nname: Nicholas\ndays:\n  - \"06.12\": feast day\n    \"p+7\": translation\n  - \"09.05\"\n---\n\n"
        "Bishop of *Myra*.\n\nMore.\n\n");
    const auto s = load_one(file);
    CHECK(s.name == "Nicholas" && s.description == "Bishop of *Myra*.\n\nMore." && s.file == file);
    CHECK(s.days.size() == 3 && s.days[0].title == "feast day" && s.days[0].day.month == 12);
    CHECK(s.days[1].title == "translation" && s.days[1].day.kind == bcal::Day::Kind::Pascha);
    CHECK(s.days[2].title.empty() && s.days[2].day.day == 9);
    CHECK(load_one(sb.write("e.md", "---\r\nname: A\r\ndays: [\"01.01\"]\r\n---\r\n")).description.empty());
    auto bad = [&](const char* name, const std::string& text, const char* msg) {
      try { load_one(sb.write(name, text)); } catch (const std::invalid_argument& e) {
        return std::string(e.what()).find(msg) != std::string::npos;
      }
      return false;
    };
    CHECK(bad("b1.md", "no header\n", "b1.md:1: the file must start"));
    CHECK(bad("b2.md", "---\nname: A\n", "b2.md:1: the header is not closed"));
    CHECK(bad("b3.md", "---\nname: [\n---\n", "YAML syntax error"));
    CHECK(bad("b4.md", "---\ndays: [\"01.01\"]\n---\n", "name must be non-empty"));
    CHECK(bad("b5.md", "---\nname: A\n---\n", "days is missing"));
    CHECK(bad("b6.md", "---\nname: A\ndays: []\n---\n", "b6.md:3: days must be a list"));
    CHECK(bad("b7.md", "---\nname: A\ndays:\n  - \"31.04\"\n---\n", "b7.md:4: invalid date"));
    CHECK(bad("b8.md", "---\nname: A\ndays:\n  - \"01.01\": \"\"\n---\n", "b8.md:4: the title"));
    CHECK(bad("b9.md", "---\nname: A\ndays:\n  - [1]\n---\n", "b9.md:4: a day must be"));
    CHECK(load_one(sb.write("t.md", "---\nname: A\ntype: event\ndays: [\"01.01\"]\n---\n")).type == "event");
    CHECK(bad("b16.md", "---\nname: A\ntype: \"\"\ndays: [\"01.01\"]\n---\n", "b16.md:3: type must be"));
    CHECK(bad("b14.md", "---\nname: A\nlang:\ndays: [\"01.01\"]\n---\n", "b14.md:3: lang must be"));
    CHECK(load_one(sb.write("l.md", "---\nname: A\nlang: de_DE\ndays: [\"01.01\"]\n---\n")).lang == "de_DE");
    CHECK(load_one(sb.write("nl.md", "---\nname: A\ndays: [\"01.01\"]\n---\n")).lang.empty());
    CHECK(bad("b15.md", "---\nname: A\ndays: [\"01.01\"]\nlang: [de]\n---\n", "b15.md:4: lang must be"));
    CHECK(bad("b10.md", "---\n- a\n---\n", "must be a map"));
    CHECK(bad("b11.md", "---\nname: A\ndays:\n  - \"01.01\":\n---\n", "b11.md:4: the title"));
    CHECK(bad("b12.md", "---\nname:\ndays: [\"01.01\"]\n---\n", "b12.md:2: name must be"));
    CHECK(bad("b13.md", "---\r\nname: A\r\ndays: [\"31.04\"]\r\n---\r\n", "b13.md:3: invalid date"));
    CHECK(load_one(sb.write("bom.md", "\xEF\xBB\xBF---\nname: A\ndays: [05.10]\n---\nx\n---\ny\n")).description == "x\n---\ny");
    {
      const auto many = bcal::load_entry_file(sb.write("m.yaml",
          "name: A\ntype: event\ndays: [\"01.01\"]\n---\nname: B\nlang: de_DE\ndays:\n  - \"p+1\": x\n"));
      CHECK(many.size() == 2 && many[0].name == "A" && many[0].type == "event" && many[1].name == "B");
      CHECK(many[0].description.empty() && many[1].lang == "de_DE" && many[1].days[0].title == "x");
      CHECK(bcal::load_entry_file(sb.write("none.yaml", "")).empty());
    }
    CHECK(bad("y1.yaml", "name: A\ndays: [\"01.01\"]\n---\nname: B\ndays: [\"31.04\"]\n", "y1.yaml:5: invalid date"));
    CHECK(bad("y2.yaml", "name: A\ndays: [\"01.01\"]\n---\n- x\n", "y2.yaml:4: an entry must be a map"));
    CHECK(bad("y3.yaml", "name: [\n", "y3.yaml:"));
    CHECK(bad("y4.yaml", "name: A\n", "y4.yaml:1: days is missing"));
    CHECK(throws<bcal::ConfigFileError>([&] { load_one(sb.dir / "nope.md"); }));
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::load_entry_file(sb.dir / "nope.yaml"); }));
    CHECK(throws<bcal::ConfigFileError>([&] { load_one(sb.dir); }));
  }

  {
    Sandbox sb;
    sb.write("entries/b.md", "---\nname: B\ndays: [\"02.01\"]\n---\n");
    sb.write("entries/a.md", "---\nname: A\ndays: [\"01.01\"]\n---\n");
    sb.write("entries/notes.txt", "ignored");
    sb.write("entries/events.yaml", "name: Y1\ndays: [\"06.01\"]\n---\nname: Y2\ndays: [\"07.01\"]\n");
    sb.write("entries/Z.md", "---\nname: Z\ndays: [\"03.01\"]\n---\n");
    sb.write("entries/dir.md/x", "");
    std::filesystem::create_symlink("loop.md", sb.dir / "entries/loop.md");
    const auto entries = bcal::load_entries(sb.dir / "entries");
    CHECK(entries.size() == 5 && entries[0].name == "Z" && entries[1].name == "A" && entries[2].name == "B" && entries[3].name == "Y1" && entries[4].name == "Y2");
    CHECK(bcal::load_entries(sb.dir).empty());   // no entry file
    CHECK(throws<bcal::ConfigFileError>([&] { bcal::load_entries(sb.dir / "nope"); }));
    sb.write("entries/de.md", "---\nname: D\nlang: de_DE\ndays: [\"04.01\"]\n---\n");
    sb.write("entries/en.md", "---\nname: E\nlang: en_US\ndays: [\"05.01\"]\n---\n");
    auto names = [&](const std::optional<std::string>& l) {
      std::string out;
      for (const auto& s : bcal::load_entries(sb.dir / "entries", l)) out += s.name + s.lang + ";";
      return out;
    };
    CHECK(names(std::nullopt) == "Z;A;B;Dde_DE;Een_US;Y1;Y2;");
    CHECK(names("de_DE") == "Z;A;B;Dde_DE;Y1;Y2;" && names("en_US") == "Z;A;B;Een_US;Y1;Y2;" && names("de_AT") == "Z;A;B;Y1;Y2;");
    sb.write("entries/bad.md", "---\nname: X\nlang: fr_FR\ndays: []\n---\n");
    CHECK(throws<std::invalid_argument>([&] { bcal::load_entries(sb.dir / "entries", "de_DE"); }));   // invalid although ignored
    std::filesystem::remove(sb.dir / "entries/bad.md");
    std::filesystem::remove(sb.dir / "entries/loop.md");
    sb.write("entries/c.md", "broken");
    CHECK(throws<std::invalid_argument>([&] { bcal::load_entries(sb.dir / "entries"); }));
  }

  {
    auto entry = [](const char* name, std::vector<bcal::EntryDay> days) {
      bcal::Entry s;
      s.name = name;
      s.days = std::move(days);
      return s;
    };
    bcal::YearCalendar c(2026, 13);
    const bcal::Entry& n = c.add_entry(entry("Nicholas", {{D("25.12"), "feast"}, {D("p+7"), "relics"}, {D("30.02"), "never"}}));
    c.add_entry(entry("Ann", {{D("25.12"), ""}, {D("01.01"), ""}}));
    CHECK(n.name == "Nicholas" && n.days.size() == 3);
    CHECK(c.at(1, 7).entries.size() == 2 && c.at(1, 7).entries[0] == &n);      // 25.12 + 13 days, in the order added
    CHECK(c.at(1, 7).entries[1]->name == "Ann" && c.at(1, 14).entries[0]->name == "Ann");  // 01.01 + 13
    CHECK(c.at(4, 19).entries.size() == 1 && c.at(4, 19).entries[0] == &n);        // Pascha 12.04 + 7
    CHECK(c.at(1, 8).entries.empty() && !c.at(1, 7).has_rule());                                      // separate from the rules
    c.add_day(D("25.12"), "x");
    CHECK(c.at(1, 7).has_rule() && c.at(1, 7).entries.size() == 2);
    int total = 0;
    for (const auto& d : c) total += static_cast<int>(d.entries.size());
    CHECK(total == 4);                                                                               // 30.02 never exists
    // the entries survive a move, the days still point to them
    bcal::YearCalendar moved(std::move(c));
    CHECK(moved.at(1, 7).entries[0]->name == "Nicholas" && moved.at(1, 7).entries[0]->days.size() == 3);
    bcal::YearCalendar twice(2026);
    twice.add_entry(entry("Twice", {{D("01.03"), "a"}, {D("01.03"), "b"}}));
    CHECK(twice.at(3, 1).entries.size() == 1);   // listed once
    bcal::YearCalendar leap(2028);                       // 29.02 exists
    leap.add_entry(entry("Leap", {{D("29.02"), ""}, {D("p+3<fri"), ""}}));
    CHECK(leap.at(2, 29).entries.size() == 1 && leap.at(2, 29).entries[0]->name == "Leap");
    bcal::YearCalendar leap13(2028, 13);
    leap13.add_entry(entry("Leap", {{D("29.02"), ""}}));
    CHECK(leap13.at(3, 13).entries.size() == 1 && leap13.at(2, 29).entries.empty());
    bcal::YearCalendar plain(2027);
    plain.add_entry(entry("Leap", {{D("29.02"), ""}}));
    plain.add_entry(entry("None", {}));
    int none = 0;
    for (const auto& d : plain) none += static_cast<int>(d.entries.size());
    CHECK(none == 0);
    // move assignment keeps the pointers valid
    bcal::YearCalendar target(2026), source(2027);
    source.add_entry(entry("Moved", {{D("01.01"), ""}}));
    target = std::move(source);
    CHECK(target.year() == 2027 && target.at(1, 1).entries[0]->name == "Moved");
    // a day of the previous year's date shifted into this year
    bcal::YearCalendar y(2027, 13);
    y.add_entry(entry("Dec", {{D("25.12"), ""}}));
    y.add_entry(entry("Prev", {{D("31.12"), ""}}));
    CHECK(y.at(1, 7).entries.size() == 1 && y.at(1, 13).entries.size() == 1);
    CHECK(y.at(12, 25).entries.empty() && y.at(12, 31).entries.empty());   // 25.12 and 31.12 of 2027 are in 2028
  }

  // --- invalid input / parameters
  CHECK(throws<std::invalid_argument>([] { cal("- \"31.04\": x\n", 2026); }));
  for (int bad : {0, 10000}) CHECK(throws<std::invalid_argument>([&] { cal("[]", bad); }));
  for (int ok : {1, 2012, 2100, 9999}) CHECK(!throws<std::invalid_argument>([&] { cal("[]", ok); }));

  std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
  return failures ? 1 : 0;
}
