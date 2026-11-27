#include "language.h"

#include <regex>

#include <yaml-cpp/yaml.h>

namespace bcal {

std::vector<std::string> locale_names(const std::string& locale) {
  std::string name = locale.substr(0, locale.find('@'));  // modifier
  name = name.substr(0, name.find('.'));                  // codeset
  if (locale.empty() || name == "C" || name == "POSIX") return {"en"};

  static const std::regex pattern("([a-z]{2,3})(_[A-Z]{2})?");
  std::smatch m;
  if (!std::regex_match(name, m, pattern))
    throw ConfigError("invalid locale \"" + locale + "\" (write it like de or de_DE)");

  std::vector<std::string> names;
  if (m[2].matched) names.push_back(name);
  names.push_back(m[1]);
  if (names.back() != "en") names.push_back("en");
  return names;
}

std::string choose_locale(const std::optional<std::string>& from_cli,
                          const std::optional<std::string>& from_config,
                          const std::function<const char*(const char*)>& get_env) {
  for (const auto* given : {&from_cli, &from_config}) {
    if (!*given) continue;
    locale_names(**given);  // validates
    return **given;
  }
  for (const char* variable : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
    const char* value = get_env(variable);
    if (!value || !*value) continue;
    try {
      locale_names(value);
      return value;
    } catch (const ConfigError&) {
      return "en";  // the first one that is set decides; a garbage value means no language
    }
  }
  return "en";
}

namespace {

// A list of exactly `count` names.
std::vector<std::string> names_of(const std::filesystem::path& file, const YAML::Node& list,
                                  const char* what, std::size_t count) {
  if (!list.IsSequence() || list.size() != count)
    throw ConfigError(location(file, list) + what + " must be a list of " + std::to_string(count) + " names");
  std::vector<std::string> names;
  for (const auto& item : list) {
    if (!item.IsScalar() || item.Scalar().find_first_not_of(" \t\r\n") == std::string::npos)
      throw ConfigError(location(file, item) + "the entries of " + what + " must be names");
    names.push_back(item.Scalar());
  }
  return names;
}

}  // namespace

Language::Language(const std::vector<std::filesystem::path>& files) {
  // From the least specific file to the most specific one, so that the latter win.
  for (auto file = files.rbegin(); file != files.rend(); ++file) {
    const YAML::Node root = load_yaml_file(*file, "language file");
    if (root.IsNull()) continue;
    if (!root.IsMap()) throw ConfigError(location(*file, root) + "a language file must be a map");

    for (const auto& entry : root) {
      const std::string key = entry.first.IsScalar() ? entry.first.Scalar() : "";
      if (key == "months") {
        months_ = names_of(*file, entry.second, "months", 12);
      } else if (key == "weekdays") {
        weekdays_ = names_of(*file, entry.second, "weekdays", 7);
      } else if (key == "ui") {
        if (!entry.second.IsMap()) throw ConfigError(location(*file, entry.second) + "ui must be a map");
        for (const auto& text : entry.second) {
          if (!text.first.IsScalar() || !text.second.IsScalar())
            throw ConfigError(location(*file, text.first) + "the entries of ui must be texts");
          ui_[text.first.Scalar()] = text.second.Scalar();
        }
      } else {
        throw ConfigError(location(*file, entry.first) + "unknown key \"" + key + "\" (use months, weekdays, ui)");
      }
    }
  }
  if (months_.empty() || weekdays_.empty())
    throw ConfigError(std::string("the language files have no ") + (months_.empty() ? "months" : "weekdays"));
}

}  // namespace bcal
