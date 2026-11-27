#include "language.h"

#include <regex>

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

namespace bcal {

std::vector<std::string> locale_names(const std::string& locale) {
  std::string name = locale.substr(0, locale.find('@'));  // modifier
  name = name.substr(0, name.find('.'));                  // codeset
  if (locale.empty() || name == "C" || name == "POSIX") return {"en"};

  // Any name is fine that is safe in a file name. Names like de_DE are only a convention.
  static const std::regex pattern("[A-Za-z0-9]+([_-][A-Za-z0-9]+)*");
  if (!std::regex_match(name, pattern))
    throw ConfigError("invalid language \"" + locale + "\" (use letters, digits, _ and -, e.g. de or de_DE)");

  std::vector<std::string> names{name};
  for (auto cut = name.rfind('_'); cut != std::string::npos; cut = name.rfind('_')) {
    name.resize(cut);  // foo_BAR_baz -> foo_BAR -> foo
    names.push_back(name);
  }
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

Language::Language(const std::filesystem::path& file, const LanguageFileFinder& find) {
  std::set<std::filesystem::path> read;
  load(file, find, read);
  if (months_.empty() || weekdays_.empty())
    throw ConfigError(file.string() + ": no " + (months_.empty() ? "months" : "weekdays") +
                      " in the language file or its fallbacks");
}

void Language::load(const std::filesystem::path& file, const LanguageFileFinder& find,
                    std::set<std::filesystem::path>& read) {
  std::error_code ec;
  if (!read.insert(std::filesystem::weakly_canonical(file, ec)).second) {
    spdlog::warn("language file {} was already read and is not read again (circular fallback?)", file.string());
    return;
  }
  const YAML::Node root = load_yaml_file(file, "language file");
  if (root.IsNull()) return;
  if (!root.IsMap()) throw ConfigError(location(file, root) + "a language file must be a map");

  // The fallback first, so that the keys of this file replace its keys.
  if (const YAML::Node fallback = root["fallback"]; fallback.IsDefined()) {
    if (!fallback.IsScalar()) throw ConfigError(location(file, fallback) + "fallback must be a language name");
    std::filesystem::path next;
    try {
      locale_names(fallback.Scalar());  // validates
      next = find(fallback.Scalar());
    } catch (const ConfigFileError& e) {
      throw ConfigFileError(file.string() + ": fallback: " + e.what());
    } catch (const ConfigError& e) {
      throw ConfigError(location(file, fallback) + e.what());
    }
    load(next, find, read);
  }

  for (const auto& entry : root) {
    const std::string key = entry.first.IsScalar() ? entry.first.Scalar() : "";
    if (key == "months") {
      months_ = names_of(file, entry.second, "months", 12);
    } else if (key == "weekdays") {
      weekdays_ = names_of(file, entry.second, "weekdays", 7);
    } else if (key == "ui") {
      if (!entry.second.IsMap()) throw ConfigError(location(file, entry.second) + "ui must be a map");
      for (const auto& text : entry.second) {
        if (!text.first.IsScalar() || !text.second.IsScalar())
          throw ConfigError(location(file, text.first) + "the entries of ui must be texts");
        ui_[text.first.Scalar()] = text.second.Scalar();
      }
    } else if (key != "fallback") {
      throw ConfigError(location(file, entry.first) + "unknown key \"" + key +
                        "\" (use fallback, months, weekdays, ui)");
    }
  }
}

}  // namespace bcal
