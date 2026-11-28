#include "saint.h"

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

// The header starts on line 2, so a node on line n (1-based) of the header is on line n + 1.
int line_of(const YAML::Node& n) { return n.Mark().is_null() ? 1 : n.Mark().line + 2; }

// The line of a key of the header: the position of an empty value is the next token.
int key_line(const YAML::Node& map, const char* key) {
  for (const auto& kv : map)
    if (kv.first.IsScalar() && kv.first.Scalar() == key) return line_of(kv.first);
  return 2;
}

}  // namespace

Saint load_saint(const fs::path& file) {
  std::error_code ec;
  std::ifstream in;
  if (fs::is_regular_file(file, ec)) in.open(file);
  if (!in.is_open()) throw ConfigFileError("cannot read saint " + file.string());

  std::string line, header, body;
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
  std::stringstream rest;
  rest << in.rdbuf();
  body = rest.str();

  YAML::Node root;
  try {
    root = YAML::Load(header);
  } catch (const YAML::Exception& e) {
    fail(file, e.mark.is_null() ? 1 : e.mark.line + 2, "YAML syntax error: " + e.msg);
  }
  if (!root.IsMap()) fail(file, 2, "the header must be a map with name and feast_days");

  Saint saint;
  saint.file = file;
  saint.description = trim_lines(body);

  const auto name = root["name"];
  if (!name || !name.IsScalar() || name.Scalar().empty())
    fail(file, key_line(root, "name"), "name must be non-empty text");
  saint.name = name.Scalar();

  const auto days = root["feast_days"];
  if (!days) fail(file, 2, "feast_days is missing");
  if (!days.IsSequence() || days.size() == 0)
    fail(file, key_line(root, "feast_days"), "feast_days must be a list with at least one date");

  auto add = [&](const YAML::Node& at, const std::string& date, const std::string& title) {
    const auto parsed = parse_day(date);
    if (!parsed.day) fail(file, line_of(at), "invalid date: " + parsed.error);
    saint.feasts.push_back({*parsed.day, title});
  };
  for (const auto& item : days) {
    if (item.IsScalar()) {
      add(item, item.Scalar(), "");
    } else if (item.IsMap()) {
      for (const auto& kv : item) {
        if (!kv.first.IsScalar()) fail(file, line_of(kv.first), "a date must be text");
        if (!kv.second.IsScalar() || kv.second.Scalar().empty())
          fail(file, line_of(kv.first), "the title of \"" + kv.first.Scalar() + "\" must be non-empty text");
        add(kv.first, kv.first.Scalar(), kv.second.Scalar());
      }
    } else {
      fail(file, line_of(item), "a feast day must be a date or \"date: title\"");
    }
  }
  return saint;
}

std::vector<Saint> load_saints(const fs::path& dir) {
  std::error_code ec;
  if (!fs::is_directory(dir, ec)) throw ConfigFileError("cannot read saints directory " + dir.string());
  std::vector<fs::path> files;
  for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
    if (it->path().extension() == ".md" && it->is_regular_file(ec)) files.push_back(it->path());
  if (ec) throw ConfigFileError("cannot read saints directory " + dir.string());
  std::sort(files.begin(), files.end());
  std::vector<Saint> saints;
  for (const auto& file : files) saints.push_back(load_saint(file));
  return saints;
}

}  // namespace bcal
