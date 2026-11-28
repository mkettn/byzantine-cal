#include "byzantine_cal_config.h"

#include <yaml-cpp/yaml.h>


namespace bcal {

namespace fs = std::filesystem;

YAML::Node load_yaml_file(const fs::path& file, const char* kind) {
  std::vector<YAML::Node> documents;
  try {
    std::error_code ec;
    if (!fs::is_regular_file(file, ec)) throw YAML::BadFile(file.string());
    documents = YAML::LoadAllFromFile(file.string());
  } catch (const YAML::BadFile&) {
    throw ConfigFileError(std::string("cannot read ") + kind + " " + file.string());
  } catch (const YAML::Exception& e) {
    throw ConfigError(file.string() + ":" + std::to_string(e.mark.line + 1) +
                      ": YAML syntax error: " + e.msg);
  }
  if (documents.size() > 1)
    throw ConfigError(file.string() + ": the " + kind + " must be a single YAML document");
  return documents.empty() ? YAML::Node() : documents[0];
}

std::string location(const fs::path& file, const YAML::Node& node) {
  const auto line = node.IsDefined() && !node.Mark().is_null() ? node.Mark().line + 1 : 0;
  return file.string() + (line > 0 ? ":" + std::to_string(line) : "") + ": ";
}

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
  const YAML::Node root = load_yaml_file(file, "configuration file");
  if (root.IsNull()) return;  // an empty file: no options
  if (!root.IsMap())
    throw ConfigError(location(file, root) + "the configuration must be a map of options");

  // A list of directories, absolute or relative to the directory of the file.
  auto dirs_of = [&](const YAML::Node& list, const char* option) {
    if (!list.IsSequence())
      throw ConfigError(location(file, list) + option + " must be a list of directories");
    std::vector<fs::path> dirs;
    for (const auto& item : list) {
      if (!item.IsScalar() || item.Scalar().empty())
        throw ConfigError(location(file, item) + "the entries of " + option + " must be directory names");
      const fs::path dir = item.Scalar();
      dirs.push_back(dir.is_absolute() ? dir : (file.parent_path() / dir).lexically_normal());
    }
    return dirs;
  };

  std::vector<std::string> seen;
  for (const auto& option : root) {
    const std::string name = option.first.IsScalar() ? option.first.Scalar() : "";
    if (name != "template_dir" && name != "lang_dir" && name != "language")
      throw ConfigError(location(file, option.first) + "unknown option" +
                        (option.first.IsScalar() ? " \"" + name + "\"" : ""));
    for (const auto& earlier : seen)
      if (earlier == name) throw ConfigError(location(file, option.first) + name + " is set twice");
    seen.push_back(name);

    if (name == "template_dir") {
      template_dirs_ = dirs_of(option.second, "template_dir");
    } else if (name == "lang_dir") {
      lang_dirs_ = dirs_of(option.second, "lang_dir");
    } else {
      if (!option.second.IsScalar())
        throw ConfigError(location(file, option.second) + "language must be the name of a language file");
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

fs::path ByzantineCalConfig::find_language_file(const std::string& language) const {
  for (const auto& dir : lang_dirs_)
    if (fs::is_regular_file(dir / (language + ".yaml"))) return dir / (language + ".yaml");
  std::string dirs;
  for (const auto& dir : lang_dirs_) dirs += (dirs.empty() ? "" : ", ") + dir.string();
  throw ConfigFileError("cannot find the language file " + language + ".yaml in the lang_dir of " +
                        file_.string() + (dirs.empty() ? ": it lists no directory" : " (" + dirs + ")"));
}

}  // namespace bcal
