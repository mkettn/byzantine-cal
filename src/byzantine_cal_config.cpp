#include "byzantine_cal_config.h"

#include <yaml-cpp/yaml.h>

#include "language.h"

namespace bcal {

namespace fs = std::filesystem;

namespace {

// "file:line: " for messages.
std::string where(const fs::path& file, const YAML::Node& node) {
  const auto line = node.IsDefined() && !node.Mark().is_null() ? node.Mark().line + 1 : 0;
  return file.string() + (line > 0 ? ":" + std::to_string(line) : "") + ": ";
}

}  // namespace

fs::path ByzantineCalConfig::find_file(const std::optional<std::string>& path_from_cli,
                                       const std::string& installed_file) {
  if (path_from_cli) return *path_from_cli;
  for (const fs::path& candidate : {fs::path("bcal.yaml"), fs::path(installed_file)}) {
    // Whatever is there is used, also a directory or a broken symlink: the
    // constructor reports it, instead of silently taking the next candidate.
    std::error_code ec;
    if (fs::exists(fs::symlink_status(candidate, ec))) return candidate;
  }
  throw ConfigFileError(
      "no configuration file found: looked for bcal.yaml in the current directory and " +
      installed_file);
}

ByzantineCalConfig::ByzantineCalConfig(const fs::path& file) : file_(file) {
  std::vector<YAML::Node> documents;
  try {
    std::error_code ec;
    if (!fs::is_regular_file(file, ec)) throw YAML::BadFile(file.string());
    documents = YAML::LoadAllFromFile(file.string());
  } catch (const YAML::BadFile&) {
    throw ConfigFileError("cannot read configuration file " + file.string());
  } catch (const YAML::Exception& e) {
    throw ConfigError(file.string() + ":" + std::to_string(e.mark.line + 1) +
                      ": YAML syntax error: " + e.msg);
  }

  if (documents.size() > 1)
    throw ConfigError(file.string() + ": the configuration must be a single YAML document");
  if (documents.empty() || documents[0].IsNull()) return;  // an empty file: no options
  const YAML::Node& root = documents[0];
  if (!root.IsMap())
    throw ConfigError(where(file, root) + "the configuration must be a map of options");

  // A list of directories, absolute or relative to the directory of the file.
  auto dirs_of = [&](const YAML::Node& list, const char* option) {
    if (!list.IsSequence())
      throw ConfigError(where(file, list) + option + " must be a list of directories");
    std::vector<fs::path> dirs;
    for (const auto& item : list) {
      if (!item.IsScalar() || item.Scalar().empty())
        throw ConfigError(where(file, item) + "the entries of " + option + " must be directory names");
      const fs::path dir = item.Scalar();
      dirs.push_back(dir.is_absolute() ? dir : (file.parent_path() / dir).lexically_normal());
    }
    return dirs;
  };

  std::vector<std::string> seen;
  for (const auto& option : root) {
    const std::string name = option.first.IsScalar() ? option.first.Scalar() : "";
    if (name != "template_dir" && name != "lang_dir" && name != "language")
      throw ConfigError(where(file, option.first) + "unknown option" +
                        (option.first.IsScalar() ? " \"" + name + "\"" : ""));
    for (const auto& earlier : seen)
      if (earlier == name) throw ConfigError(where(file, option.first) + name + " is set twice");
    seen.push_back(name);

    if (name == "template_dir") {
      template_dirs_ = dirs_of(option.second, "template_dir");
    } else if (name == "lang_dir") {
      lang_dirs_ = dirs_of(option.second, "lang_dir");
    } else {
      if (!option.second.IsScalar() || option.second.Scalar().empty())
        throw ConfigError(where(file, option.second) + "language must be a locale name like de_DE");
      try {
        locale_names(option.second.Scalar());
      } catch (const ConfigError& e) {
        throw ConfigError(where(file, option.second) + e.what());
      }
      language_ = option.second.Scalar();
    }
  }
}

fs::path ByzantineCalConfig::find_template(const std::string& name) const {
  for (const auto& dir : template_dirs_)
    if (fs::is_regular_file(dir / name)) return dir / name;
  std::string dirs;
  for (const auto& dir : template_dirs_) dirs += (dirs.empty() ? "" : ", ") + dir.string();
  throw ConfigFileError("cannot find the template " + name + " in the template_dir of " +
                        file_.string() + (dirs.empty() ? " (it lists no directory)" : " (" + dirs + ")"));
}

std::vector<fs::path> ByzantineCalConfig::find_language_files(const std::string& locale) const {
  std::vector<fs::path> files;
  for (const auto& name : locale_names(locale)) {
    bool found = false;
    for (const auto& dir : lang_dirs_) {
      if (fs::is_regular_file(dir / (name + ".yaml"))) {
        files.push_back(dir / (name + ".yaml"));
        found = true;
        break;
      }
    }
    if (!found && name == "en") {  // the fallback of all languages must be there
      std::string dirs;
      for (const auto& dir : lang_dirs_) dirs += (dirs.empty() ? "" : ", ") + dir.string();
      throw ConfigFileError("cannot find the language file en.yaml in the lang_dir of " +
                            file_.string() + (dirs.empty() ? " (it lists no directory)" : " (" + dirs + ")"));
    }
  }
  return files;
}

}  // namespace bcal
