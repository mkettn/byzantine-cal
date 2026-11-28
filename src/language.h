#pragma once
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "byzantine_cal_config.h"

namespace bcal {

using LanguageFileFinder = std::function<std::filesystem::path(const std::string& language)>;

class Language {
 public:
  Language(const std::filesystem::path& file, const LanguageFileFinder& find);

  const std::string& month(unsigned number) const { return months_.at(number - 1); }      // 1..12
  const std::string& weekday(unsigned number) const { return weekdays_.at(number - 1); }  // 1 = Monday .. 7
  const nlohmann::json& ui() const { return ui_; }

 private:
  void load(const std::filesystem::path& file, const LanguageFileFinder& find,
            std::set<std::filesystem::path>& read);

  std::vector<std::string> months_, weekdays_;
  nlohmann::json ui_ = nlohmann::json::object();
};

}  // namespace bcal
