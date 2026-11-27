#pragma once
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace YAML {
class Node;
}

namespace bcal {

// The configuration file is invalid.
class ConfigError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

// The configuration file cannot be found or read.
class ConfigFileError : public ConfigError {
  using ConfigError::ConfigError;
};

// Helpers to read the YAML files of the program (configuration, language files):
// the single YAML document of `file` (a null node for an empty file); `kind`
// names the file in messages. Throws ConfigFileError if it cannot be read and
// ConfigError for a syntax error or several documents.
YAML::Node load_yaml_file(const std::filesystem::path& file, const char* kind);
// "file:line: " of a node, for messages.
std::string location(const std::filesystem::path& file, const YAML::Node& node);

// The configuration of byzantine-cal, read from a YAML file:
//
//   template_dir:        # directories to look for templates, in this order
//     - /usr/share/tpl   # absolute, or
//     - tpl              # relative to the directory of the configuration file
//                        # (the directory of the given path, symlinks are not followed)
//   lang_dir: [lang]     # directories to look for language files (de_DE.yaml, ...), same rules
//   language: de_DE      # the language to use (a locale name without codeset)
//
// Unknown options, an option that is set twice and several YAML documents are
// an error.
class ByzantineCalConfig {
 public:
  // The configuration file to use: `path_from_cli` (-C) if given, otherwise
  // bcal.yaml in the current directory, otherwise `installed_file`. A path is
  // returned as it is if there is something at it, also if that is unusable (a
  // directory, a broken symlink): the constructor reports that. Throws
  // ConfigFileError if there is no configuration file at all.
  static std::filesystem::path find_file(
      const std::optional<std::string>& path_from_cli,
      const std::string& installed_file);

  // Reads the configuration file. Throws ConfigFileError if it cannot be read
  // and ConfigError if its content is invalid (messages name file and line).
  explicit ByzantineCalConfig(const std::filesystem::path& file);

  const std::filesystem::path& file() const { return file_; }

  // The template directories, absolute or relative to the working directory,
  // in the order of the configuration.
  const std::vector<std::filesystem::path>& template_dirs() const {
    return template_dirs_;
  }

  // The language to use if the configuration sets one (a valid locale name).
  const std::optional<std::string>& language() const { return language_; }

  // The language file for `locale`: for the names of locale_names() in order
  // ("de_DE", "de"), the first lang_dir that has <name>.yaml. Throws
  // ConfigFileError if there is none and ConfigError for an invalid locale.
  std::filesystem::path find_language_file(const std::string& locale) const;

  // The path of the template file `name` in the first template directory that
  // has it, ready to be given to a template backend. Throws ConfigFileError if
  // no directory has it.
  std::filesystem::path find_template(const std::string& name) const;

 private:
  std::filesystem::path file_;
  std::vector<std::filesystem::path> template_dirs_;
  std::vector<std::filesystem::path> lang_dirs_;
  std::optional<std::string> language_;
};

}  // namespace bcal
