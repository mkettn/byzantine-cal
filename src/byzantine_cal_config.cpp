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

  bool template_dir_seen = false;
  for (const auto& option : root) {
    if (!option.first.IsScalar() || option.first.Scalar() != "template_dir")
      throw ConfigError(where(file, option.first) + "unknown option" +
                        (option.first.IsScalar() ? " \"" + option.first.Scalar() + "\"" : ""));
    if (template_dir_seen) throw ConfigError(where(file, option.first) + "template_dir is set twice");
    template_dir_seen = true;
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
