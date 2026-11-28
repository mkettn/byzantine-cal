#include "entry.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <yaml-cpp/yaml.h>

#include "byzantine_cal_config.h"

namespace bcal {

namespace {

namespace fs = std::filesystem;

[[noreturn]] void fail(const fs::path& file, int line, const std::string& message) {
  throw std::invalid_argument(file.string() + ":" + std::to_string(line) + ": " + message);
}

bool is_fence(const std::string& line) {
  return line.rfind("---", 0) == 0 && line.find_first_not_of(" \t\r", 3) == std::string::npos;
}

std::string trim_lines(const std::string& text) {
  const auto first = text.find_first_not_of("\r\n");
  if (first == std::string::npos) return "";
  return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

// A node of the header is on line mark + base: the header of a .md file starts on line 2, a
// YAML document of a .yaml file on line 1.
int line_of(const YAML::Node& n, int base) { return n.Mark().is_null() ? base : n.Mark().line + base; }

// The line of a key: the position of an empty value is the next token.
int key_line(const YAML::Node& map, const char* key, int base) {
  for (const auto& kv : map)
    if (kv.first.IsScalar() && kv.first.Scalar() == key) return line_of(kv.first, base);
  return line_of(map, base);
}

Entry parse_entry(const fs::path& file, const YAML::Node& root, int base) {
  if (!root.IsMap()) fail(file, line_of(root, base), "an entry must be a map with name and days");

  Entry entry;
  entry.file = file;

  const auto name = root["name"];
  if (!name || !name.IsScalar() || name.Scalar().empty())
    fail(file, key_line(root, "name", base), "name must be non-empty text");
  entry.name = name.Scalar();

  for (const auto& [key, target] : {std::pair<const char*, std::string*>{"type", &entry.type},
                                    std::pair<const char*, std::string*>{"lang", &entry.lang}}) {
    const auto value = root[key];
    if (!value) continue;
    if (!value.IsScalar() || value.Scalar().empty())
      fail(file, key_line(root, key, base), std::string(key) + " must be non-empty text");
    *target = value.Scalar();
  }

  const auto days = root["days"];
  if (!days) fail(file, line_of(root, base), "days is missing");
  if (!days.IsSequence() || days.size() == 0)
    fail(file, key_line(root, "days", base), "days must be a list with at least one date");

  auto add = [&](const YAML::Node& at, const std::string& date, const std::string& title) {
    const auto parsed = parse_day(date);
    if (!parsed.day) fail(file, line_of(at, base), "invalid date: " + parsed.error);
    entry.days.push_back({*parsed.day, title});
  };
  for (const auto& item : days) {
    if (item.IsScalar()) {
      add(item, item.Scalar(), "");
    } else if (item.IsMap()) {
      for (const auto& kv : item) {
        if (!kv.first.IsScalar()) fail(file, line_of(kv.first, base), "a date must be text");
        if (!kv.second.IsScalar() || kv.second.Scalar().empty())
          fail(file, line_of(kv.first, base), "the title of \"" + kv.first.Scalar() + "\" must be non-empty text");
        add(kv.first, kv.first.Scalar(), kv.second.Scalar());
      }
    } else {
      fail(file, line_of(item, base), "a day must be a date or \"date: title\"");
    }
  }
  return entry;
}

std::vector<Entry> load_markdown(const fs::path& file) {
  std::ifstream in;
  std::error_code ec;
  if (fs::is_regular_file(file, ec)) in.open(file);
  if (!in.is_open()) throw ConfigFileError("cannot read " + file.string());

  std::string line, header;
  const bool has_line = static_cast<bool>(std::getline(in, line));
  if (line.rfind("\xEF\xBB\xBF", 0) == 0) line.erase(0, 3);  // a byte order mark
  if (!has_line || !is_fence(line))
    fail(file, 1, "the file must start with a header between two lines of ---");
  bool closed = false;
  while (std::getline(in, line)) {
    if (is_fence(line)) {
      closed = true;
      break;
    }
    header += line + "\n";
  }
  if (!closed) fail(file, 1, "the header is not closed by a line of ---");
  std::stringstream body;
  body << in.rdbuf();

  YAML::Node root;
  try {
    root = YAML::Load(header);
  } catch (const YAML::Exception& e) {
    fail(file, e.mark.is_null() ? 1 : e.mark.line + 2, "YAML syntax error: " + e.msg);
  }
  Entry entry = parse_entry(file, root, 2);
  entry.description = trim_lines(body.str());
  return {entry};
}

std::vector<Entry> load_yaml(const fs::path& file) {
  std::vector<YAML::Node> documents;
  std::error_code ec;
  try {
    if (!fs::is_regular_file(file, ec)) throw YAML::BadFile(file.string());
    documents = YAML::LoadAllFromFile(file.string());
  } catch (const YAML::BadFile&) {
    throw ConfigFileError("cannot read " + file.string());
  } catch (const YAML::Exception& e) {
    fail(file, e.mark.is_null() ? 1 : e.mark.line + 1, "YAML syntax error: " + e.msg);
  }
  std::vector<Entry> entries;
  for (const auto& document : documents)
    if (!document.IsNull()) entries.push_back(parse_entry(file, document, 1));  // empty: a trailing ---
  return entries;
}

}  // namespace

std::vector<Entry> load_entry_file(const fs::path& file) {
  return file.extension() == ".yaml" ? load_yaml(file) : load_markdown(file);
}

std::vector<Entry> load_entries(const fs::path& dir, const std::optional<std::string>& language) {
  std::error_code ec;
  if (!fs::is_directory(dir, ec)) throw ConfigFileError("cannot read directory " + dir.string());
  std::vector<fs::path> files;
  for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
    const auto extension = it->path().extension();
    if ((extension == ".md" || extension == ".yaml") && it->is_regular_file(ec)) files.push_back(it->path());
  }
  if (ec) throw ConfigFileError("cannot read directory " + dir.string());
  std::sort(files.begin(), files.end());
  std::vector<Entry> entries;
  for (const auto& file : files)
    for (auto& entry : load_entry_file(file))  // also an entry of another language has to be valid
      if (!language || entry.lang.empty() || entry.lang == *language) entries.push_back(std::move(entry));
  return entries;
}

}  // namespace bcal
