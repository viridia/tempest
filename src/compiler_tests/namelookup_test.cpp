#include "catch.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/specstore.hpp"
#include "tempest/sema/names/membernamelookup.hpp"
#include "tempest/support/allocator.hpp"
#include <memory>

using namespace tempest::sema::graph;
using namespace tempest::sema::names;
using namespace tempest::source;
using namespace tempest::support;

TEST_CASE("MemberNameLookup", "[names]") {
  Location loc;
  BumpPtrAllocator alloc;
  SpecializationStore sp(alloc);

  SECTION("Module") {
    Module m("TestModule");
    ValueDefn v(Member::Kind::VAR_DEF, loc, "x");
    m.memberScope()->addMember(&v);
    MemberLookupResult result;
    MemberNameLookup lookup(sp);

    lookup.lookup("x", &m, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("y", &m, result);
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

    ValueDefn v(Member::Kind::VAR_DEF, loc, "x");
    td.memberScope()->addMember(&v);
    MemberLookupResult result;
    MemberNameLookup lookup(sp);

    lookup.lookup("x", &td, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, result,
        MemberNameLookup::INHERITED_ONLY | MemberNameLookup::INSTANCE_MEMBERS);
    REQUIRE(result.size() == 0);

    result.clear();
    lookup.lookup("y", &td, result);
    REQUIRE(result.size() == 0);

    result.clear();
    lookup.lookup("y", &testCls, result);
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

    ValueDefn v(Member::Kind::VAR_DEF, loc, "x");
    baseTypeDef.memberScope()->addMember(&v);

    MemberLookupResult result;
    MemberNameLookup lookup(sp);

    lookup.lookup("x", &td, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, result);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, result, MemberNameLookup::INSTANCE_MEMBERS);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, result,
        MemberNameLookup::INSTANCE_MEMBERS | MemberNameLookup::INHERITED_ONLY);
    REQUIRE(result.size() == 1);

    result.clear();
    lookup.lookup("x", &testCls, result,
        MemberNameLookup::STATIC_MEMBERS | MemberNameLookup::INHERITED_ONLY);
    REQUIRE(result.size() == 0);

    result.clear();
    lookup.lookup("y", &td, result);
    REQUIRE(result.size() == 0);

    result.clear();
    lookup.lookup("y", &testCls, result);
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
