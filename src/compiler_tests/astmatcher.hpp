#include "catch.hpp"
#include "tempest/ast/node.hpp"
#include "tempest/ast/ident.hpp"
#include "tempest/ast/literal.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/ast/oper.hpp"
#include <iostream>
#include <llvm/ADT/SmallVector.h>

/** Match the AST structure against it's string representation. */
class ASTStrEquals : public Catch::MatcherBase<const tempest::ast::Node*> {
public:
  ASTStrEquals(llvm::StringRef expected) : _expected(expected) {}

  virtual bool match(const tempest::ast::Node* node) const override {
    std::stringstream strm;
    tempest::ast::format(strm, node, true);
    if (strm.str() != _expected) {
      std::cerr << "Actual value:\n";
      std::string s(strm.str());
      llvm::StringRef actual(s);
      llvm::SmallVector<llvm::StringRef, 16> lines;
      actual.split(lines, '\n');
      for (auto l: lines) {
        std::cerr << '"' << l << "\\n\"\n";
      }
      return false;
    }
    return true;
    // return !nodeDiff.visit(_expected, node);
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "Expected to equal:\n" << _expected;
    return ss.str();
  }

private:
  llvm::StringRef _expected;
};

inline ASTStrEquals ASTEQ(llvm::StringRef expected) {
  return ASTStrEquals(expected);
}
