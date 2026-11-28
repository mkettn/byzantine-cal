#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "language.h"
#include "year_calendar.h"

namespace bcal {

nlohmann::json calendar_to_json(const YearCalendar& calendar, const Language& language);

class TemplateError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class TemplateFileError : public TemplateError {
  using TemplateError::TemplateError;
};

std::string escape_markdown(const std::string& text);

std::string render_template(const YearCalendar& calendar, const Language& language,
                            const std::string& template_text);

class TemplateBackend {
 public:
  explicit TemplateBackend(const std::filesystem::path& template_path);

  std::string render(const YearCalendar& calendar, const Language& language) const {
    return render_template(calendar, language, template_);
  }

 private:
  std::string template_;
};

class MarkdownBackend : public TemplateBackend {
 public:
  static constexpr const char* kTemplateFile = "markdown.inja";

  explicit MarkdownBackend(const std::filesystem::path& template_path)
      : TemplateBackend(template_path) {}
};

}  // namespace bcal
