#pragma once
#include "cae/string_pool.hpp"
#include "cae/types.hpp"

#include <cmath>
#include <cctype>
#include <memory>
#define CAE_HAS_EXPRTK 0
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace cae {

class IExprEngine {
 public:
  virtual ~IExprEngine() = default;
  virtual void set(std::string name, double value) = 0;
  virtual double eval(const std::string& expr) = 0;
  virtual double get(std::string_view name) const = 0;

  void setVariable(std::string name, double value) { set(std::move(name), value); }
  double evaluate(const std::string& expr) { return eval(expr); }
};

class SimpleExprEngine final : public IExprEngine {
 public:
  void set(std::string name, double value) override { vars_[std::move(name)] = value; }
  double get(std::string_view name) const override {
    auto it = vars_.find(std::string(name));
    return it == vars_.end() ? 0.0 : it->second;
  }
  double eval(const std::string& expr) override {
    Parser p{*this, expr};
    return p.parse_expression();
  }

 private:
  struct Parser {
    const SimpleExprEngine& eng;
    std::string_view s;
    size_t i = 0;

    void skip_ws() { while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; }
    bool match(char c) {
      skip_ws();
      if (i < s.size() && s[i] == c) { ++i; return true; }
      return false;
    }

    std::string parse_ident() {
      skip_ws();
      size_t b = i;
      if (i < s.size() && (std::isalpha(static_cast<unsigned char>(s[i])) || s[i] == '_')) ++i;
      while (i < s.size() && (std::isalnum(static_cast<unsigned char>(s[i])) || s[i] == '_')) ++i;
      return std::string(s.substr(b, i - b));
    }

    double parse_number() {
      skip_ws();
      size_t b = i;
      if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
      bool dot = false;
      while (i < s.size()) {
        char c = s[i];
        if (std::isdigit(static_cast<unsigned char>(c))) { ++i; continue; }
        if (c == '.' && !dot) { dot = true; ++i; continue; }
        break;
      }
      if (b == i) throw std::runtime_error("expected number");
      return std::stod(std::string(s.substr(b, i - b)));
    }

    double parse_factor() {
      skip_ws();
      if (match('(')) { double v = parse_expression(); if (!match(')')) throw std::runtime_error("missing )"); return v; }
      if (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '+' || s[i] == '-')) {
        size_t save = i;
        try { return parse_number(); } catch (...) { i = save; }
      }

      std::string id = parse_ident();
      if (id.empty()) throw std::runtime_error("expected factor");
      skip_ws();
      if (match('(')) {
        double a = parse_expression();
        if (!match(',')) throw std::runtime_error("missing ,");
        double b = parse_expression();
        if (!match(')')) throw std::runtime_error("missing )");
        if (id == "max") return std::max(a, b);
        if (id == "min") return std::min(a, b);
        throw std::runtime_error("unknown function");
      }
      return eng.get(id);
    }

    double parse_term() {
      double v = parse_factor();
      while (true) {
        skip_ws();
        if (match('*')) v *= parse_factor();
        else if (match('/')) v /= parse_factor();
        else break;
      }
      return v;
    }

    double parse_expression() {
      double v = parse_term();
      while (true) {
        skip_ws();
        if (match('+')) v += parse_term();
        else if (match('-')) v -= parse_term();
        else break;
      }
      return v;
    }
  };

  std::unordered_map<std::string, double> vars_;
};

#if __has_include(<exprtk.hpp>)
#undef CAE_HAS_EXPRTK
#define CAE_HAS_EXPRTK 1
#include <exprtk.hpp>
class ExprTkEngine final : public IExprEngine {
 public:
  void set(std::string name, double value) override { vars_[std::move(name)] = value; dirty_ = true; }
  double get(std::string_view name) const override {
    auto it = vars_.find(std::string(name));
    return it == vars_.end() ? 0.0 : it->second;
  }
  double eval(const std::string& expr) override {
    auto& cached = cache_[expr];
    if (dirty_ || !cached.compiled) {
      st_.clear();
      backing_.clear();
      for (const auto& [k, v] : vars_) {
        backing_[k] = v;
        st_.add_variable(k, backing_[k]);
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
  std::unordered_map<std::string, double> vars_;
  std::unordered_map<std::string, Compiled> cache_;
  std::unordered_map<std::string, double> backing_;
  exprtk::symbol_table<double> st_;
  bool dirty_ = true;
};
#endif

class ParamTable {
 public:
  explicit ParamTable(const StringPool* strings = nullptr) : strings_(strings)
#if CAE_HAS_EXPRTK
      , engine_(std::make_unique<ExprTkEngine>())
#else
      , engine_(std::make_unique<SimpleExprEngine>())
#endif
  {}

  void bind_string_pool(const StringPool* strings) { strings_ = strings; }

  void set_var(std::string name, double value) { engine_->set(std::move(name), value); }
  void set_var(StringId name, double value) {
    if (!strings_) return;
    engine_->set(std::string(strings_->view(name)), value);
  }

  double eval(const std::string& expression) { return engine_->eval(expression); }
  double get_var(std::string_view name) const { return engine_->get(name); }

  int64_t resolve(DbuValue v) const {
    if (!v.is_param()) return v.lit();
    if (!strings_) return 0;
    auto expr = strings_->view(v.pid());
    if (expr.empty()) return 0;
    return static_cast<int64_t>(std::llround(const_cast<IExprEngine*>(engine_.get())->eval(std::string(expr))));
  }

  // Grok-style compatibility API
  void setVariable(std::string name, double value) { set_var(std::move(name), value); }
  double evaluate(const std::string& expression) { return eval(expression); }

 private:
  const StringPool* strings_ = nullptr;
  std::unique_ptr<IExprEngine> engine_;
};

}  // namespace cae
