#include "catch.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include <memory>

using namespace tempest::sema::graph;

TEST_CASE("Primitive Types", "[type]") {
  SECTION("VoidType") {
    REQUIRE(VoidType::VOID.name() == "void");
  }

  SECTION("BooleanType") {
    REQUIRE(BooleanType::BOOL.name() == "bool");
  }

  SECTION("IntegerType") {
    REQUIRE(IntegerType::CHAR.name() == "char");
    REQUIRE(IntegerType::CHAR.bits() == 32);
    REQUIRE(IntegerType::CHAR.isUnsigned());
    REQUIRE(IntegerType::I8.name() == "i8");
    REQUIRE(IntegerType::I8.bits() == 8);
    REQUIRE(!IntegerType::I8.isUnsigned());
    REQUIRE(IntegerType::I16.name() == "i16");
    REQUIRE(IntegerType::I16.bits() == 16);
    REQUIRE(!IntegerType::I16.isUnsigned());
    REQUIRE(IntegerType::I32.name() == "i32");
    REQUIRE(IntegerType::I32.bits() == 32);
    REQUIRE(!IntegerType::I32.isUnsigned());
    REQUIRE(IntegerType::I64.name() == "i64");
    REQUIRE(IntegerType::I64.bits() == 64);
    REQUIRE(!IntegerType::I64.isUnsigned());
    REQUIRE(IntegerType::U8.name() == "u8");
    REQUIRE(IntegerType::U8.bits() == 8);
    REQUIRE(IntegerType::U8.isUnsigned());
    REQUIRE(IntegerType::U16.name() == "u16");
    REQUIRE(IntegerType::U16.bits() == 16);
    REQUIRE(IntegerType::U16.isUnsigned());
    REQUIRE(IntegerType::U32.name() == "u32");
    REQUIRE(IntegerType::U32.bits() == 32);
    REQUIRE(IntegerType::U32.isUnsigned());
    REQUIRE(IntegerType::U64.name() == "u64");
    REQUIRE(IntegerType::U64.isUnsigned());
    REQUIRE(IntegerType::U64.bits() == 64);
  }

  SECTION("FloatType") {
    REQUIRE(FloatType::F32.name() == "f32");
    REQUIRE(FloatType::F32.bits() == 32);
    REQUIRE(FloatType::F64.name() == "f64");
    REQUIRE(FloatType::F64.bits() == 64);
  }
}
