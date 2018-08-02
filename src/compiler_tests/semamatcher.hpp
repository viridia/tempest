#include "catch.hpp"
#include "tempest/sema/graph/defn.hpp"
#include <iostream>
#include <llvm/ADT/SmallVector.h>

/** Match the AST structure against it's string representation. */
class MemberStrEquals : public Catch::MatcherBase<const tempest::sema::graph::Member*> {
public:
  MemberStrEquals(llvm::StringRef expected) : _expected(expected) {}

  virtual bool match(const tempest::sema::graph::Member* m) const override {
    std::stringstream strm;
    tempest::sema::graph::format(strm, m, true);
    if (strm.str() != _expected) {
      std::cerr << "Actual value:\n";
      std::string s(strm.str());
      llvm::StringRef actual(s);
      llvm::SmallVector<llvm::StringRef, 16> lines;
      actual.split(lines, '\n');
      for (auto l: lines) {
        std::cerr << "    \"" << l << "\\n\"\n";
      }
      return false;
    }
    return true;
  }

  virtual std::string describe() const override {
    std::ostringstream ss;
    ss << "Expected to equal:\n" << _expected;
    return ss.str();
  }

private:
  llvm::StringRef _expected;
};

inline MemberStrEquals MemberEQ(llvm::StringRef expected) {
  return MemberStrEquals(expected);
}
