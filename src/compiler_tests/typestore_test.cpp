#include "catch.hpp"
#include "tempest/sema/graph/primitivetype.h"
#include "tempest/sema/graph/typestore.h"

using namespace tempest::sema::graph;

TEST_CASE("TypeKey", "[type]") {
  SECTION("Ordered") {
    TypeKey tk1({ &IntegerType::I16, &IntegerType::I32 });
    TypeKey tk2({ &IntegerType::I32, &IntegerType::I16 });
    REQUIRE(tk1 != tk2);
  }
}

TEST_CASE("TypeStore", "[type]") {
  TypeStore ts;

  SECTION("UnionType") {
    const UnionType* ut1 = ts.createUnionType({ &IntegerType::I16, &IntegerType::I32 });
    const UnionType* ut2 = ts.createUnionType({ &IntegerType::I32, &IntegerType::I16 });
    REQUIRE(ut1 == ut2);
    REQUIRE(ut1->members[0] == &IntegerType::I16);
    REQUIRE(ut1->members[1] == &IntegerType::I32);
  }

  SECTION("TupleType") {
    const TupleType* tt1 = ts.createTupleType({ &IntegerType::I16, &IntegerType::I32 });
    const TupleType* tt2 = ts.createTupleType({ &IntegerType::I32, &IntegerType::I16 });
    REQUIRE(tt1 != tt2);
    REQUIRE(tt1->members[0] == &IntegerType::I16);
    REQUIRE(tt1->members[1] == &IntegerType::I32);
    REQUIRE(tt2->members[0] == &IntegerType::I32);
    REQUIRE(tt2->members[1] == &IntegerType::I16);
  }
}
