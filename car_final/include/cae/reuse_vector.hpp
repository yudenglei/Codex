#pragma once
#include "cae/types.hpp"

#include <vector>

namespace cae {
template <typename T>
class ReuseVector {
 public:
  Id add(const T& v) {
    if (!free_.empty()) {
      const Id idx = free_.back();
      free_.pop_back();
      data_[idx - 1] = v;
      used_[idx - 1] = true;
      return idx;
    }
    data_.push_back(v);
    used_.push_back(true);
    return static_cast<Id>(data_.size());
  }
  bool remove(Id id) {
    if (!contains(id)) return false;
    used_[id - 1] = false;
    free_.push_back(id);
    return true;
  }
  bool replace(Id id, const T& v) {
    if (!contains(id)) return false;
    data_[id - 1] = v;
    return true;
  }
  T* get(Id id) { return contains(id) ? &data_[id - 1] : nullptr; }
  const T* get(Id id) const { return contains(id) ? &data_[id - 1] : nullptr; }
  bool contains(Id id) const { return id > 0 && id <= used_.size() && used_[id - 1]; }

  template <typename F>
  void for_each(F&& f) const {
    for (Id i = 1; i <= data_.size(); ++i) if (used_[i - 1]) f(i, data_[i - 1]);
  }

 private:
  std::vector<T> data_;
  std::vector<bool> used_;
  std::vector<Id> free_;
};
}  // namespace cae
