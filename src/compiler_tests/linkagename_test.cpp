#include "catch.hpp"
#include "tempest/sema/graph/primitivetype.h"
#include "tempest/sema/graph/typestore.h"
#include "tempest/gen/linkagename.h"
#include <memory>

using namespace tempest::sema::graph;
using namespace tempest::gen;
using tempest::source::Location;

TEST_CASE("LinkageName", "[gen]") {
  TypeStore ts;
  std::string name;

  SECTION("type void") {
    getLinkageName(name, &VoidType::VOID);
    REQUIRE(name == "void");
  }

  SECTION("type bool") {
    getLinkageName(name, &BooleanType::BOOL);
    REQUIRE(name == "bool");
  }

  SECTION("type i16") {
    getLinkageName(name, &IntegerType::I16);
    REQUIRE(name == "i16");
  }

  SECTION("type i32") {
    getLinkageName(name, &IntegerType::I32);
    REQUIRE(name == "i32");
  }

  SECTION("type u32") {
    getLinkageName(name, &IntegerType::U32);
    REQUIRE(name == "u32");
  }

  SECTION("type f32") {
    getLinkageName(name, &FloatType::F32);
    REQUIRE(name == "f32");
  }

  SECTION("type f64") {
    getLinkageName(name, &FloatType::F64);
    REQUIRE(name == "f64");
  }

  SECTION("UnionType") {
    const UnionType* ut1 = ts.createUnionType({ &IntegerType::I16, &IntegerType::I32 });
    getLinkageName(name, ut1);
    REQUIRE(name == "i16|i32");
    const UnionType* ut2 = ts.createUnionType({ &FloatType::F32, &IntegerType::I32 });
    name.clear();
    getLinkageName(name, ut2);
    REQUIRE(name == "i32|f32");
  }

  SECTION("TupleType") {
    const TupleType* tt1 = ts.createTupleType({
      &IntegerType::I16, &IntegerType::I32, &IntegerType::I32 });
    getLinkageName(name, tt1);
    REQUIRE(name == "(i16,i32,i32)");
  }

  SECTION("defn void") {
    getLinkageName(name, VoidType::VOID.defn());
    REQUIRE(name == "void");
  }

  SECTION("Class") {
    TypeDefn clsDefn(Location(), "A");
    getLinkageName(name, &clsDefn);
    REQUIRE(name == "A");
  }

  SECTION("Inner Class") {
    TypeDefn clsDefn(Location(), "A");
    TypeDefn innerDefn(Location(), "B", &clsDefn);
    getLinkageName(name, &innerDefn);
    REQUIRE(name == "A.B");
  }

  SECTION("Specialized Class") {
    TypeParameter tpS(Location(), "S", 0);
    TypeParameter tpT(Location(), "T", 1);
    TypeDefn clsDefn(Location(), "A");
    clsDefn.typeParams().push_back(&tpS);
    clsDefn.typeParams().push_back(&tpT);
    SpecializedDefn specDefn(&clsDefn, { &IntegerType::I16, &IntegerType::I32 });
    getLinkageName(name, &specDefn);
    REQUIRE(name == "A[i16,i32]");
  }
}
