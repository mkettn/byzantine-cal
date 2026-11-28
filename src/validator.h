#pragma once
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace bcal {

struct Issue {
  int line = 0;  // 1-based, 0 = unknown
  std::string message;
};

struct Report {
  std::vector<Issue> errors;
  int entries = 0;
  bool ok() const { return errors.empty(); }
};

Report validate(const YAML::Node& root);

YAML::Node load_file(const std::string& path, Report& report);

Report validate_file(const std::string& path);

bool is_valid_rule(const std::string& rule);

}  // namespace bcal
