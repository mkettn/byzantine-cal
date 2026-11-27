#pragma once
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "year_calendar.h"

namespace bcal {

// The calendar as handed to templates:
//   year, offset_days, pascha ("YYYY-MM-DD"),
//   days:   [{date ("YYYY-MM-DD"), year, month, day, weekday (1 = Monday ..
//             7 = Sunday), weekday_name, has_rule, data}]
//             (data is "" for days without a rule)
//   months: [{number, name, rule_count, days: [...same as above...]}]
nlohmann::json calendar_to_json(const YearCalendar& calendar);

class TemplateError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

// Renders an inja template with the data of calendar_to_json(). Besides the
// inja built-ins, the function replace(text, from, to) is available. Blocks
// ({% ... %}) that are alone on their line leave no empty line behind. Line
// statements start with "%%" (not inja's default "##", a Markdown heading).
// Throws TemplateError if the template is invalid or cannot be rendered.
std::string render_template(const YearCalendar& calendar,
                            const std::string& template_text);

}  // namespace bcal
