#include "catch.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/names/membernamelookup.hpp"
#include <memory>

using namespace tempest::sema::graph;
using namespace tempest::sema::names;
using namespace tempest::source;

TEST_CASE("MemberNameLookup", "[names]") {
  Location loc;

  SECTION("Module") {
    Module m("TestModule");
    ValueDefn v(Member::Kind::CONST_DEF, loc, "x");
    m.memberScope()->addMember(&v);
    NameLookupResult result;
    MemberNameLookup lookup;

    lookup.lookup("x", &m, false, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("y", &m, false, result);
    REQUIRE(result.size() == 0);

    size_t count = 0;
    lookup.forAllNames(&m, [&count] (const llvm::StringRef& name) {
      count += 1;
    });
    REQUIRE(count == 1);
  }

  SECTION("Type") {
    TypeDefn td(loc, "TestTypeDefn");
    UserDefinedType testCls(Type::Kind::CLASS);
    testCls.setDefn(&td);
    td.setType(&testCls);

    ValueDefn v(Member::Kind::CONST_DEF, loc, "x");
    td.memberScope()->addMember(&v);
    NameLookupResult result;
    MemberNameLookup lookup;

    lookup.lookup("x", &td, false, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, false, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("y", &td, false, result);
    REQUIRE(result.size() == 0);

    result.clear();
    lookup.lookup("y", &testCls, false, result);
    REQUIRE(result.size() == 0);

    size_t count = 0;
    lookup.forAllNames(&testCls, [&count] (const llvm::StringRef& name) {
      count += 1;
    });
    REQUIRE(count == 1);
  }

  SECTION("Inherited Type") {
    TypeDefn td(loc, "TestTypeDefn");
    UserDefinedType testCls(Type::Kind::CLASS);
    testCls.setDefn(&td);
    td.setType(&testCls);

    TypeDefn baseTypeDef(loc, "Base");
    UserDefinedType baseCls(Type::Kind::CLASS);
    baseCls.setDefn(&baseTypeDef);
    baseTypeDef.setType(&baseCls);

    td.extends().push_back(&baseTypeDef);

    ValueDefn v(Member::Kind::CONST_DEF, loc, "x");
    baseTypeDef.memberScope()->addMember(&v);

    NameLookupResult result;
    MemberNameLookup lookup;

    lookup.lookup("x", &td, false, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, false, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("y", &td, false, result);
    REQUIRE(result.size() == 0);

    result.clear();
    lookup.lookup("y", &testCls, false, result);
    REQUIRE(result.size() == 0);

    size_t count = 0;
    lookup.forAllNames(&testCls, [&count] (const llvm::StringRef& name) {
      count += 1;
    });
    REQUIRE(count == 1);
  }

      // case Member::Kind::TYPE_PARAM: {
      //   auto tp = static_cast<TypeParameter*>(stem);
      //   for (auto st : tp->subtypeConstraints()) {
      //     lookup(name, st, fromStatic, result);
      //   }
      //   break;
      // }

}
