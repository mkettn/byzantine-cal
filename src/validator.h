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
  int entries = 0;  // number of date / range entries checked
  bool ok() const { return errors.empty(); }
};

// Validates a parsed calendar document (a sequence of maps).
Report validate(const YAML::Node& root);

// Loads and validates a file. YAML syntax errors become a Report error;
// unreadable files (or directories) throw YAML::BadFile.
Report validate_file(const std::string& path);

bool is_valid_rule(const std::string& rule);

}  // namespace bcal
