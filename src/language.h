#pragma once
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "byzantine_cal_config.h"

namespace bcal {

// The names of the language files to look for a language, most specific first.
// A language is a name that is safe in a file name (letters, digits, _ and -);
// names like the locales of setlocale() (de_DE) are only a convention. A codeset
// (".UTF-8", implied) and a "@modifier" are ignored. The name is shortened at
// the last "_" step by step: "de_DE" -> {"de_DE", "de"}, "foo_BAR" -> {"foo_BAR",
// "foo"}. "C", "POSIX" and "" mean no language: {"en"}. Throws ConfigError for
// any other name.
std::vector<std::string> locale_names(const std::string& locale);

// The language to use: `from_cli` (--lang), else `from_config` (language:), else
// the first non-empty of LC_ALL, LC_MESSAGES and LANG (English if it is not a
// valid name), else "en". Throws ConfigError for an invalid `from_cli` or
// `from_config`.
std::string choose_locale(const std::optional<std::string>& from_cli,
                          const std::optional<std::string>& from_config,
                          const std::function<const char*(const char*)>& get_env = std::getenv);

// Finds the language file of a locale, throws ConfigFileError if there is none
// (see ByzantineCalConfig::find_language_file()).
using LanguageFileFinder = std::function<std::filesystem::path(const std::string& locale)>;

// The words of a language, read from a language file (YAML): `months` (12
// names), `weekdays` (7 names, Monday first), `ui` (texts for templates) and
// `fallback` (a language name, optional). Every key is optional, but `months` and
// `weekdays` must be complete if present.
//
// The `fallback` language (found with `find`) is read before the file itself, so
// that the keys of the file replace those of the fallback (`ui` entry by entry),
// and so on along the fallbacks of the fallbacks. A file is read only once: a
// second reference to it (a circular fallback) is ignored with a warning.
class Language {
 public:
  // Throws ConfigFileError if a file cannot be read or found and ConfigError if
  // it is invalid or the months or the weekdays are missing at the end.
  Language(const std::filesystem::path& file, const LanguageFileFinder& find);

  const std::string& month(unsigned number) const { return months_.at(number - 1); }      // 1..12
  const std::string& weekday(unsigned number) const { return weekdays_.at(number - 1); }  // 1 = Monday .. 7
  const nlohmann::json& ui() const { return ui_; }   // object of strings

 private:
  void load(const std::filesystem::path& file, const LanguageFileFinder& find,
            std::set<std::filesystem::path>& read);

  std::vector<std::string> months_, weekdays_;
  nlohmann::json ui_ = nlohmann::json::object();
};

}  // namespace bcal
