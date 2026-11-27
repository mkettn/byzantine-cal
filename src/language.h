#pragma once
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "byzantine_cal_config.h"

namespace bcal {

// The names of the language files to look for a locale, most specific first,
// ending with "en". A locale is written as for setlocale() without the codeset
// (".UTF-8" is implied; it and a "@modifier" are ignored): "de_DE" -> {"de_DE",
// "de", "en"}, "de" -> {"de", "en"}. "C", "POSIX" and "" mean no language: {"en"}.
// Throws ConfigError for anything else (2-3 lowercase letters, optionally _ and
// 2 uppercase letters).
std::vector<std::string> locale_names(const std::string& locale);

// The locale to use: `from_cli` (--lang), else `from_config` (language:), else
// the first non-empty of LC_ALL, LC_MESSAGES and LANG (English if it is not a
// valid locale), else "en". Throws ConfigError for an invalid `from_cli` or
// `from_config`.
std::string choose_locale(const std::optional<std::string>& from_cli,
                          const std::optional<std::string>& from_config,
                          const std::function<const char*(const char*)>& get_env = std::getenv);

// The words of a language, read from language files (YAML): `months` (12
  // names), `weekdays` (7 names, Monday first) and `ui` (texts for templates).
// Every key is optional in a file, but if present it must be complete.
class Language {
 public:
  // `files`: most specific first (see ByzantineCalConfig::find_language_files()).
  // A key that a file leaves out comes from the next one, `ui` is merged by
  // entry. Throws ConfigFileError if a file cannot be read and ConfigError if
  // it is invalid or no file has the months or the weekdays.
  explicit Language(const std::vector<std::filesystem::path>& files);

  const std::string& month(unsigned number) const { return months_.at(number - 1); }      // 1..12
  const std::string& weekday(unsigned number) const { return weekdays_.at(number - 1); }  // 1 = Monday .. 7
  const nlohmann::json& ui() const { return ui_; }   // object of strings

 private:
  std::vector<std::string> months_, weekdays_;
  nlohmann::json ui_ = nlohmann::json::object();
};

}  // namespace bcal
