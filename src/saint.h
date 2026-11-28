#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "day.h"

namespace bcal {

struct Feast {
  Day day;
  std::string title;  // empty without one
};

struct Saint {
  std::string name;
  std::string description;  // the Markdown after the header
  std::vector<Feast> feasts;
  std::filesystem::path file;
};

// Throws std::invalid_argument, with the line, for an invalid file and ConfigFileError if it
// cannot be read.
Saint load_saint(const std::filesystem::path& file);

// All *.md files of the directory, sorted by file name. ConfigFileError if it cannot be read.
std::vector<Saint> load_saints(const std::filesystem::path& dir);

}  // namespace bcal
