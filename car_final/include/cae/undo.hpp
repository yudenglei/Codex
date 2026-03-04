#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace cae {
class UndoRedo {
 public:
  struct Op { std::function<void()> undo; std::function<void()> redo; std::string label; };

  void begin_tx(std::string label) {
    if (in_tx_) return;
    in_tx_ = true;
    tx_label_ = std::move(label);
    batch_.clear();
  }

  void push(Op op) {
    if (in_tx_) { batch_.push_back(std::move(op)); return; }
    trim_redo_tail();
    ops_.push_back(std::move(op));
    cursor_ = ops_.size();
  }

  void commit_tx() {
    if (!in_tx_) return;
    if (!batch_.empty()) {
      Op merged;
      merged.label = tx_label_;
      auto ops = std::move(batch_);
      merged.undo = [ops]() mutable { for (auto it = ops.rbegin(); it != ops.rend(); ++it) it->undo(); };
      merged.redo = [ops]() mutable { for (auto& op : ops) op.redo(); };
      in_tx_ = false;
      push(std::move(merged));
      return;
    }
    in_tx_ = false;
  }

  void rollback_tx() {
    if (!in_tx_) return;
    for (auto it = batch_.rbegin(); it != batch_.rend(); ++it) it->undo();
    batch_.clear();
    in_tx_ = false;
  }

  bool undo() {
    if (cursor_ == 0) return false;
    --cursor_;
    ops_[cursor_].undo();
    return true;
  }
  bool redo() {
    if (cursor_ >= ops_.size()) return false;
    ops_[cursor_].redo();
    ++cursor_;
    return true;
  }

 private:
  void trim_redo_tail() { if (cursor_ < ops_.size()) ops_.erase(ops_.begin() + static_cast<long>(cursor_), ops_.end()); }

  std::vector<Op> ops_;
  std::vector<Op> batch_;
  size_t cursor_ = 0;
  bool in_tx_ = false;
  std::string tx_label_;
};
}  // namespace cae
