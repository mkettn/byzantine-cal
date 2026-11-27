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
  if (from_cli) {
    locale_names(*from_cli);  // validates
    return *from_cli;
  }
  if (from_config) {
    locale_names(*from_config);
    return *from_config;
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

std::string where(const std::filesystem::path& file, const YAML::Node& node) {
  const auto line = node.IsDefined() && !node.Mark().is_null() ? node.Mark().line + 1 : 0;
  return file.string() + (line > 0 ? ":" + std::to_string(line) : "") + ": ";
}

// A list of exactly `count` non-empty names.
template <std::size_t N>
std::array<std::string, N> names_of(const std::filesystem::path& file, const YAML::Node& list,
                                    const char* what) {
  if (!list.IsSequence() || list.size() != N)
    throw ConfigError(where(file, list) + what + " must be a list of " + std::to_string(N) + " names");
  std::array<std::string, N> names;
  for (std::size_t i = 0; i < N; ++i) {
    if (!list[i].IsScalar() || list[i].Scalar().empty())
      throw ConfigError(where(file, list[i]) + "the entries of " + what + " must be names");
    names[i] = list[i].Scalar();
  }
  return names;
}

}  // namespace

Language::Language(const std::vector<std::filesystem::path>& files) {
  bool have_months = false, have_weekdays = false;
  // From the least specific file to the most specific one, so that the latter win.
  for (auto file = files.rbegin(); file != files.rend(); ++file) {
    std::vector<YAML::Node> documents;
    try {
      std::error_code ec;
      if (!std::filesystem::is_regular_file(*file, ec)) throw YAML::BadFile(file->string());
      documents = YAML::LoadAllFromFile(file->string());
    } catch (const YAML::BadFile&) {
      throw ConfigFileError("cannot read language file " + file->string());
    } catch (const YAML::Exception& e) {
      throw ConfigError(file->string() + ":" + std::to_string(e.mark.line + 1) +
                        ": YAML syntax error: " + e.msg);
    }
    if (documents.size() > 1)
      throw ConfigError(file->string() + ": a language file must be a single YAML document");
    if (documents.empty() || documents[0].IsNull()) continue;
    const YAML::Node& root = documents[0];
    if (!root.IsMap()) throw ConfigError(where(*file, root) + "a language file must be a map");

    for (const auto& entry : root) {
      const std::string key = entry.first.IsScalar() ? entry.first.Scalar() : "";
      if (key == "months") {
        months_ = names_of<12>(*file, entry.second, "months");
        have_months = true;
      } else if (key == "weekdays") {
        weekdays_ = names_of<7>(*file, entry.second, "weekdays");
        have_weekdays = true;
      } else if (key == "ui") {
        if (!entry.second.IsMap()) throw ConfigError(where(*file, entry.second) + "ui must be a map");
        for (const auto& text : entry.second) {
          if (!text.first.IsScalar() || !text.second.IsScalar())
            throw ConfigError(where(*file, text.first) + "the entries of ui must be texts");
          ui_[text.first.Scalar()] = text.second.Scalar();
        }
      } else {
        throw ConfigError(where(*file, entry.first) + "unknown key \"" + key +
                          "\" (use months, weekdays, ui)");
      }
    }
  }
  if (!have_months || !have_weekdays)
    throw ConfigError(std::string("the language files have no ") + (have_months ? "weekdays" : "months"));
}

}  // namespace bcal
