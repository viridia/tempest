#include "catch.hpp"
#include "tempest/sema/graph/symboltable.hpp"
#include <memory>

using tempest::sema::graph::SymbolTable;
using tempest::sema::graph::NameLookupResult;
using tempest::sema::graph::Member;

TEST_CASE("Symbol tables", "[symbol]") {
  SymbolTable sym;
  NameLookupResult res;
  auto x = std::make_unique<Member>(Member::Kind::LET_DEF, "x");
  auto y = std::make_unique<Member>(Member::Kind::LET_DEF, "y");

  SECTION("addMember") {
    sym.addMember(x.get());
    sym.addMember(y.get());
    sym.lookupName("x", res);
    REQUIRE(res.size() == 1);
    REQUIRE(res[0] == x.get());
  }

  SECTION("forAllNames") {
    size_t count = 0;
    sym.addMember(x.get());
    sym.addMember(y.get());
    sym.forAllNames([&count] (const llvm::StringRef& name) {
      count += 1;
    });
    REQUIRE(count == 2);
  }
}
