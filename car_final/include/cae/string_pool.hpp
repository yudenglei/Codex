#pragma once
#include "cae/types.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cae {
class StringPool {
 public:
  StringId intern(std::string_view s) {
    auto k = std::string(s);
    if (auto it = to_id_.find(k); it != to_id_.end()) return it->second;
    StringId id = static_cast<StringId>(values_.size() + 1);
    values_.push_back(k);
    to_id_[values_.back()] = id;
    index_full_text(id, values_.back());
    return id;
  }
  std::string_view view(StringId id) const {
    if (id == 0 || id > values_.size()) return {};
    return values_[id - 1];
  }
  std::vector<StringId> find_prefix(std::string_view pfx) const {
    std::vector<StringId> out;
    for (size_t i = 0; i < values_.size(); ++i) if (values_[i].rfind(pfx, 0) == 0) out.push_back(static_cast<StringId>(i + 1));
    return out;
  }
  std::vector<StringId> search_full_text(std::string_view token) const {
    auto t = normalize(token);
    auto it = inverted_.find(t);
    if (it == inverted_.end()) return {};
    return it->second;
  }

  template <typename F>
  void for_each(F&& f) const { for (StringId i = 1; i <= values_.size(); ++i) f(i, values_[i - 1]); }

 private:
  static std::string normalize(std::string_view s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s) o.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return o;
  }
  void index_full_text(StringId id, const std::string& s) {
    std::string tok;
    auto flush = [&]() {
      if (tok.empty()) return;
      auto& vec = inverted_[tok];
      if (vec.empty() || vec.back() != id) vec.push_back(id);
      tok.clear();
    };
    for (char c : s) {
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') tok.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
      else flush();
    }
    flush();
  }

  std::vector<std::string> values_;
  std::unordered_map<std::string, StringId> to_id_;
  std::unordered_map<std::string, std::vector<StringId>> inverted_;
};
}  // namespace cae
