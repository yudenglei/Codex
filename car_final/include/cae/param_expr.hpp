#pragma once
#include "cae/types.hpp"

#include <cmath>
#include <memory>
#define CAE_HAS_EXPRTK 0
#include <string>
#include <unordered_map>

namespace cae {

class IExprEngine {
 public:
  virtual ~IExprEngine() = default;
  virtual void set(StringId name, double value) = 0;
  virtual double eval(const std::string& expr) = 0;
  virtual double get(StringId name) const = 0;
};

class SimpleExprEngine final : public IExprEngine {
 public:
  void set(StringId name, double value) override { vars_[name] = value; }
  double eval(const std::string& expr) override {
    auto pos = expr.find('*');
    if (pos != std::string::npos) return ref(expr.substr(0, pos)) * ref(expr.substr(pos + 1));
    pos = expr.find('+');
    if (pos != std::string::npos) return ref(expr.substr(0, pos)) + ref(expr.substr(pos + 1));
    return ref(expr);
  }
  double get(StringId name) const override {
    auto it = vars_.find(name);
    return it == vars_.end() ? 0.0 : it->second;
  }

 private:
  double ref(const std::string& s) const {
    try { return std::stod(s); } catch (...) {}
    try {
      auto it = vars_.find(static_cast<StringId>(std::stoul(s)));
      return it == vars_.end() ? 0.0 : it->second;
    } catch (...) { return 0.0; }
  }
  std::unordered_map<StringId, double> vars_;
};

#if __has_include(<exprtk.hpp>)
#undef CAE_HAS_EXPRTK
#define CAE_HAS_EXPRTK 1
#include <exprtk.hpp>
class ExprTkEngine final : public IExprEngine {
 public:
  void set(StringId name, double value) override { vars_[name] = value; dirty_ = true; }
  double get(StringId name) const override {
    auto it = vars_.find(name);
    return it == vars_.end() ? 0.0 : it->second;
  }
  double eval(const std::string& expr) override {
    auto& cached = cache_[expr];
    if (dirty_ || !cached.compiled) {
      st_.clear();
      backing_.clear();
      for (const auto& [k, v] : vars_) {
        const auto key = std::to_string(k);
        backing_[key] = v;
        st_.add_variable(key, backing_[key]);
      }
      cached.expression.register_symbol_table(st_);
      exprtk::parser<double> p;
      cached.compiled = p.compile(expr, cached.expression);
      dirty_ = false;
    }
    return cached.compiled ? cached.expression.value() : 0.0;
  }

 private:
  struct Compiled { bool compiled = false; exprtk::expression<double> expression; };
  std::unordered_map<StringId, double> vars_;
  std::unordered_map<std::string, Compiled> cache_;
  std::unordered_map<std::string, double> backing_;
  exprtk::symbol_table<double> st_;
  bool dirty_ = true;
};
#endif

class ParamTable {
 public:
  ParamTable()
#if CAE_HAS_EXPRTK
      : engine_(std::make_unique<ExprTkEngine>())
#else
      : engine_(std::make_unique<SimpleExprEngine>())
#endif
  {}

  void set_var(StringId name, double value) { engine_->set(name, value); }
  double eval(const std::string& expression) { return engine_->eval(expression); }
  double get_var(StringId name) const { return engine_->get(name); }
  int64_t resolve(DbuValue v) const {
    return v.is_param() ? static_cast<int64_t>(std::llround(engine_->get(v.pid()))) : v.lit();
  }

 private:
  std::unique_ptr<IExprEngine> engine_;
};

}  // namespace cae
