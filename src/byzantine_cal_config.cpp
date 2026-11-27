#include "byzantine_cal_config.h"

#include <yaml-cpp/yaml.h>

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
  for (const fs::path& candidate : {fs::path("bcal.yaml"), fs::path(installed_file)})
    if (fs::is_regular_file(candidate)) return candidate;
  throw ConfigFileError(
      "no configuration file found: looked for bcal.yaml in the current directory and " +
      installed_file);
}

ByzantineCalConfig::ByzantineCalConfig(const fs::path& file) : file_(file) {
  YAML::Node root;
  try {
    if (!fs::is_regular_file(file)) throw YAML::BadFile(file.string());
    root = YAML::LoadFile(file.string());
  } catch (const YAML::BadFile&) {
    throw ConfigFileError("cannot read configuration file " + file.string());
  } catch (const YAML::Exception& e) {
    throw ConfigError(file.string() + ":" + std::to_string(e.mark.line + 1) +
                      ": YAML syntax error: " + e.msg);
  }

  if (root.IsNull()) return;  // an empty file: no options
  if (!root.IsMap())
    throw ConfigError(where(file, root) + "the configuration must be a map of options");

  for (const auto& option : root) {
    if (!option.first.IsScalar() || option.first.Scalar() != "template_dir")
      throw ConfigError(where(file, option.first) + "unknown option" +
                        (option.first.IsScalar() ? " \"" + option.first.Scalar() + "\"" : ""));
    if (!option.second.IsSequence())
      throw ConfigError(where(file, option.second) +
                        "template_dir must be a list of directories");
    for (const auto& item : option.second) {
      if (!item.IsScalar() || item.Scalar().empty())
        throw ConfigError(where(file, item) +
                          "the entries of template_dir must be directory names");
      const fs::path dir = item.Scalar();
      template_dirs_.push_back(dir.is_absolute() ? dir
                                                 : (file.parent_path() / dir).lexically_normal());
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

}  // namespace bcal
