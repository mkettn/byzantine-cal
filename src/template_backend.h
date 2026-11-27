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

// The template file of a backend cannot be read.
class TemplateFileError : public TemplateError {
  using TemplateError::TemplateError;
};

// Escapes text for Markdown: every character that has a meaning there
// (\ ` * _ { } [ ] ( ) < > # + - . ! | ~ &) gets a backslash, so the text is
// shown literally, also in a table cell. Newlines become <br>.
std::string escape_markdown(const std::string& text);

// Renders an inja template with the data of calendar_to_json(). Besides the
// inja built-ins, the functions replace(text, from, to) and
// escape_markdown(text) are available. Blocks
// ({% ... %}) that are alone on their line leave no empty line behind. Line
// statements start with "%%" (not inja's default "##", a Markdown heading).
// Throws TemplateError if the template is invalid or cannot be rendered.
std::string render_template(const YearCalendar& calendar,
                            const std::string& template_text);

// A backend turns a YearCalendar into text with an inja template that it reads
// from a template directory. Throws TemplateFileError if the file cannot be
// read; render() throws TemplateError (see render_template()).
class TemplateBackend {
 public:
  TemplateBackend(const std::string& template_dir,
                  const std::string& template_file);

  std::string render(const YearCalendar& calendar) const {
    return render_template(calendar, template_);
  }

 private:
  std::string template_;
};

// The Markdown backend, by default with the template markdown.inja.
class MarkdownBackend : public TemplateBackend {
 public:
  explicit MarkdownBackend(const std::string& template_dir,
                           const std::string& template_file = "markdown.inja")
      : TemplateBackend(template_dir, template_file) {}
};

}  // namespace bcal
