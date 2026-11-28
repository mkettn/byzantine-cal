#pragma once
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "byzantine_cal_config.h"

namespace bcal {

// Finds the file of a language (the value of `language:` or `fallback:`); throws
// ConfigFileError if there is none.
using LanguageFileFinder = std::function<std::filesystem::path(const std::string& language)>;

// The words of a language, read from a language file (YAML): `months` (12
// names), `weekdays` (7, Monday first), `ui` (texts for templates) and `fallback`
// (the name of a language). Every key is optional. The fallback (found with `find`) is
// read first, so the keys of the file replace its keys (`ui` entry by entry), and
// so on along the fallbacks. A file is read only once: a second reference
// (a circular fallback) is ignored with a warning.
class Language {
 public:
  // Throws ConfigFileError if a file cannot be read or found and ConfigError if
  // it is invalid or the months or weekdays are missing at the end.
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
