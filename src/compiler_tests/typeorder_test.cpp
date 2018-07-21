#include "catch.hpp"
#include "tempest/sema/graph/primitivetype.h"
#include "tempest/sema/graph/typestore.h"
#include "tempest/sema/graph/typeorder.h"
#include "tempest/gen/linkagename.h"

using namespace tempest::sema::graph;
using tempest::source::Location;

TEST_CASE("TypeOrder", "[type]") {
  TypeOrder isLessThan;
  TypeStore ts;

  SECTION("VoidType") {
    REQUIRE_FALSE(isLessThan(&VoidType::VOID, &VoidType::VOID));
    REQUIRE(isLessThan(&VoidType::VOID, &BooleanType::BOOL));
    REQUIRE_FALSE(isLessThan(&BooleanType::BOOL, &VoidType::VOID));
    REQUIRE(isLessThan(&VoidType::VOID, &IntegerType::I32));
    REQUIRE_FALSE(isLessThan(&IntegerType::I32, &VoidType::VOID));
    REQUIRE(isLessThan(&VoidType::VOID, &FloatType::F32));
    REQUIRE_FALSE(isLessThan(&FloatType::F32, &VoidType::VOID));
  }

  SECTION("BooleanType") {
    REQUIRE_FALSE(isLessThan(&BooleanType::BOOL, &BooleanType::BOOL));
    REQUIRE(isLessThan(&BooleanType::BOOL, &IntegerType::I32));
    REQUIRE_FALSE(isLessThan(&IntegerType::I32, &BooleanType::BOOL));
    REQUIRE(isLessThan(&BooleanType::BOOL, &FloatType::F32));
    REQUIRE_FALSE(isLessThan(&FloatType::F32, &BooleanType::BOOL));
  }

  SECTION("IntegerType") {
    REQUIRE_FALSE(isLessThan(&IntegerType::I8, &IntegerType::I8));
    REQUIRE_FALSE(isLessThan(&IntegerType::I16, &IntegerType::I16));
    REQUIRE_FALSE(isLessThan(&IntegerType::I32, &IntegerType::I32));
    REQUIRE_FALSE(isLessThan(&IntegerType::I64, &IntegerType::I64));
    REQUIRE_FALSE(isLessThan(&IntegerType::U8, &IntegerType::U8));
    REQUIRE_FALSE(isLessThan(&IntegerType::U16, &IntegerType::U16));
    REQUIRE_FALSE(isLessThan(&IntegerType::U32, &IntegerType::U32));
    REQUIRE_FALSE(isLessThan(&IntegerType::U64, &IntegerType::U64));
    REQUIRE(isLessThan(&IntegerType::I8, &IntegerType::I32));
    REQUIRE_FALSE(isLessThan(&IntegerType::I32, &IntegerType::I8));
    REQUIRE(isLessThan(&IntegerType::I8, &IntegerType::U32));
    REQUIRE_FALSE(isLessThan(&IntegerType::U32, &IntegerType::I8));
    REQUIRE(isLessThan(&IntegerType::I32, &FloatType::F32));
    REQUIRE_FALSE(isLessThan(&FloatType::F32, &IntegerType::I32));
  }

  SECTION("FloatType") {
    REQUIRE_FALSE(isLessThan(&FloatType::F32, &FloatType::F32));
    REQUIRE_FALSE(isLessThan(&FloatType::F64, &FloatType::F64));
    REQUIRE(isLessThan(&FloatType::F32, &FloatType::F64));
    REQUIRE_FALSE(isLessThan(&FloatType::F64, &FloatType::F32));
  }

  SECTION("UnionType") {
    REQUIRE_FALSE(isLessThan(
        ts.createUnionType({ &IntegerType::I16, &IntegerType::I32 }),
        ts.createUnionType({ &IntegerType::I16, &IntegerType::I32 })));
    REQUIRE(isLessThan(
        ts.createUnionType({ &IntegerType::I16, &IntegerType::I32 }),
        ts.createUnionType({ &IntegerType::I16, &IntegerType::I32, &IntegerType::I32 })));
    REQUIRE_FALSE(isLessThan(
        ts.createUnionType({ &IntegerType::I16, &IntegerType::I32, &IntegerType::I32 }),
        ts.createUnionType({ &IntegerType::I16, &IntegerType::I32 })));
    REQUIRE(isLessThan(
        ts.createUnionType({ &IntegerType::I16, &IntegerType::I32 }),
        ts.createUnionType({ &IntegerType::I32, &IntegerType::I32 })));
  }

  SECTION("TupleType") {
    REQUIRE_FALSE(isLessThan(
        ts.createTupleType({ &IntegerType::I16, &IntegerType::I32 }),
        ts.createTupleType({ &IntegerType::I16, &IntegerType::I32 })));
    REQUIRE(isLessThan(
        ts.createTupleType({ &IntegerType::I16, &IntegerType::I32 }),
        ts.createTupleType({ &IntegerType::I16, &IntegerType::I32, &IntegerType::I32 })));
    REQUIRE_FALSE(isLessThan(
        ts.createTupleType({ &IntegerType::I16, &IntegerType::I32, &IntegerType::I32 }),
        ts.createTupleType({ &IntegerType::I16, &IntegerType::I32 })));
    REQUIRE(isLessThan(
        ts.createTupleType({ &IntegerType::I16, &IntegerType::I32 }),
        ts.createTupleType({ &IntegerType::I32, &IntegerType::I32 })));
  }

  SECTION("Class") {
    TypeDefn clsDefnA(Location(), "A");
    UserDefinedType clsA(Type::Kind::CLASS, &clsDefnA);
    TypeDefn clsDefnB(Location(), "B");
    UserDefinedType clsB(Type::Kind::CLASS, &clsDefnB);
    REQUIRE_FALSE(isLessThan(&clsA, &clsA));
    REQUIRE(isLessThan(&clsA, &clsB));
  }

  // SECTION("Inner Class") {
  //   TypeDefn clsDefn(Location(), "A");
  //   TypeDefn innerDefn(Location(), "B", &clsDefn);
  //   getLinkageName(name, &innerDefn);
  //   REQUIRE(name == "A.B");
  // }

  // SECTION("Specialized Class") {
  //   TypeParameter tpS(Location(), "S", 0);
  //   TypeParameter tpT(Location(), "T", 1);
  //   TypeDefn clsDefn(Location(), "A");
  //   clsDefn.typeParams().push_back(&tpS);
  //   clsDefn.typeParams().push_back(&tpT);
  //   SpecializedDefn specDefn(&clsDefn, { &IntegerType::I16, &IntegerType::I32 });
  //   getLinkageName(name, &specDefn);
  //   REQUIRE(name == "A[i16,i32]");
  // }
}
