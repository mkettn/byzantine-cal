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

class ConfigError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class ConfigFileError : public ConfigError {
  using ConfigError::ConfigError;
};

YAML::Node load_yaml_file(const std::filesystem::path& file, const char* kind);
std::string location(const std::filesystem::path& file, const YAML::Node& node);

class ByzantineCalConfig {
 public:
  static std::filesystem::path find_file(
      const std::optional<std::string>& path_from_cli,
      const std::string& installed_file);

  explicit ByzantineCalConfig(const std::filesystem::path& file);

  const std::filesystem::path& file() const { return file_; }

  const std::vector<std::filesystem::path>& template_dirs() const {
    return template_dirs_;
  }

  const std::optional<std::string>& language() const { return language_; }

  std::filesystem::path find_language_file(const std::string& language) const;

  std::filesystem::path find_template(const std::string& name) const;

 private:
  std::filesystem::path file_;
  std::vector<std::filesystem::path> template_dirs_;
  std::vector<std::filesystem::path> lang_dirs_;
  std::optional<std::string> language_;
};

}  // namespace bcal
