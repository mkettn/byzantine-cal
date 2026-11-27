#pragma once
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace bcal {

// The configuration file is invalid.
class ConfigError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

// The configuration file cannot be found or read.
class ConfigFileError : public ConfigError {
  using ConfigError::ConfigError;
};

// The configuration of byzantine-cal, read from a YAML file:
//
//   template_dir:        # directories to look for templates, in this order
//     - /usr/share/tpl   # absolute, or
//     - tpl              # relative to the directory of the configuration file
//
// Unknown options are an error.
class ByzantineCalConfig {
 public:
  // The configuration file to use: `path_from_cli` (-C) if given, otherwise
  // bcal.yaml in the current directory, otherwise `installed_file`. An
  // explicit path is returned as it is (the constructor reports it if it is
  // unusable). Throws ConfigFileError if there is no configuration file.
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

  // The path of the template file `name` in the first template directory that
  // has it, ready to be given to a template backend. Throws ConfigFileError if
  // no directory has it.
  std::filesystem::path find_template(const std::string& name) const;

 private:
  std::filesystem::path file_;
  std::vector<std::filesystem::path> template_dirs_;
};

}  // namespace bcal
