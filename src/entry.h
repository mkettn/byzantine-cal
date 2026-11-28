#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "day.h"

namespace bcal {

struct EntryDay {
  Day day;                 // the first day
  std::string title;       // empty without one
  std::optional<Day> end;  // the last day of a range
};

struct Entry {
  std::string name;
  std::string type;         // for example saint or event, empty without one
  std::string lang;         // the language of the entry, empty without one
  std::string description;  // the Markdown after the header, empty in a .yaml file
  std::vector<EntryDay> days;
  std::filesystem::path file;
};

// The entries of a file: a .md file has one entry, a .yaml file one for each YAML document.
// Throws std::invalid_argument, with the line, for an invalid file and ConfigFileError if it
// cannot be read.
std::vector<Entry> load_entry_file(const std::filesystem::path& file);

// The entries of all *.md and *.yaml files of the directory, sorted by file name.
// ConfigFileError if it cannot be read. With a language, the entries with another lang: are
// left out.
std::vector<Entry> load_entries(const std::filesystem::path& dir,
                                const std::optional<std::string>& language = std::nullopt);

}  // namespace bcal
