#include "catch.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/Casting.h"
#include <iostream>

using namespace tempest::compiler;
using namespace tempest::sema::convert;
using namespace tempest::sema::graph;
using namespace tempest::sema::pass;
using namespace std;
using namespace llvm;
using tempest::parse::Parser;
using tempest::source::Location;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

namespace {
  /** Parse a module definition and apply buildgraph & nameresolution pass. */
  std::unique_ptr<Module> compile(CompilationUnit &cu, const char* srcText) {
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    return mod;
  }
}

TEST_CASE("Convert.Assignable", "[sema]") {
  CompilationUnit cu;

  SECTION("Convert integer types") {
    REQUIRE(isAssignable(&IntegerType::I8, &IntegerType::I8)
        == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(&IntegerType::I32, &IntegerType::I32)
        == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(&IntegerType::I8, &IntegerType::I16)
        == ConversionResult(ConversionRank::ERROR, ConversionError::TRUNCATION));
    REQUIRE(isAssignable(&IntegerType::I16, &IntegerType::U16)
        == ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    REQUIRE(isAssignable(&IntegerType::I16, &IntegerType::I8)
        == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(&IntegerType::I8, &FloatType::F32)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(&IntegerType::I8, &BooleanType::BOOL)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(&IntegerType::I8, &VoidType::VOID)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
  }

  SECTION("Convert implicit integer types") {
    llvm::APInt seventeen(64, 17);
    llvm::APInt oneThousand(64, 1024);
    llvm::APInt negativeOne(64, -1, true);
    auto ii17 = cu.types().createIntegerType(seventeen, false); // A 6-bit number (signed)
    auto iu17 = cu.types().createIntegerType(seventeen, true); // A 5-bit number (unsigned)
    auto ii1k = cu.types().createIntegerType(oneThousand, false); // A 12-bit number (signed)
    auto iu1k = cu.types().createIntegerType(oneThousand, true); // An 11-bit number (unsigned)
    auto iin1 = cu.types().createIntegerType(negativeOne, false); // A 2-bit number (signed)
    auto iun1 = cu.types().createIntegerType(negativeOne, true); // An 1-bit number (unsigned)
    REQUIRE(ii17->bits() == 6);
    REQUIRE(iu17->bits() == 6);
    REQUIRE(ii1k->bits() == 12);
    REQUIRE(iu1k->bits() == 12);
    REQUIRE(iin1->bits() == 1);
    REQUIRE(iun1->bits() == 1);
    // `17` fits in an I8
    REQUIRE(isAssignable(&IntegerType::I8, ii17) == ConversionResult(ConversionRank::EXACT));
    // `17` fits in a U8
    REQUIRE(isAssignable(&IntegerType::U8, ii17) == ConversionResult(ConversionRank::EXACT));
    // `17u` does not fit in an U8
    REQUIRE(isAssignable(&IntegerType::I8, iu17) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    // `17u` fits in a U8
    REQUIRE(isAssignable(&IntegerType::U8, iu17) == ConversionResult(ConversionRank::EXACT));

    // `1000` does not fit in an I8
    REQUIRE(isAssignable(&IntegerType::I8, ii1k) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::TRUNCATION));
    // `1000` does fit in an I16
    REQUIRE(isAssignable(&IntegerType::I16, ii1k) == ConversionResult(ConversionRank::EXACT));

    // `1000` does not fit in a U8
    REQUIRE(isAssignable(&IntegerType::U8, ii1k) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::TRUNCATION));
    // `1000` does fit in an U16
    REQUIRE(isAssignable(&IntegerType::U16, ii1k) == ConversionResult(ConversionRank::EXACT));

    // `1000u` does not fit in an I8
    REQUIRE(isAssignable(&IntegerType::I8, iu1k) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    // `1000u` does not fit in an I16
    REQUIRE(isAssignable(&IntegerType::I16, iu1k) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));

    // `1000u` does not fit in a U8
    REQUIRE(isAssignable(&IntegerType::U8, iu1k) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::TRUNCATION));
    // `1000u` does fit in an U16
    REQUIRE(isAssignable(&IntegerType::U16, iu1k) == ConversionResult(ConversionRank::EXACT));

    // `-1` fits in an I8
    REQUIRE(isAssignable(&IntegerType::I8, iin1) == ConversionResult(ConversionRank::EXACT));
    // `-1` fits in an I16
    REQUIRE(isAssignable(&IntegerType::I16, iin1) == ConversionResult(ConversionRank::EXACT));

    // `-1` does not fit in an U8
    REQUIRE(isAssignable(&IntegerType::U8, iin1) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    // `-1` does not fit in a U16
    REQUIRE(isAssignable(&IntegerType::U16, iin1) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));

    // `-1u` does not fit in an I8
    REQUIRE(isAssignable(&IntegerType::I8, iun1) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    // `-1u` does not fit in an I16
    REQUIRE(isAssignable(&IntegerType::I16, iun1) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));

    // `-1u` does not fit in an U8
    REQUIRE(isAssignable(&IntegerType::U8, iun1) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    // `-1u` does not fit in a U16
    REQUIRE(isAssignable(&IntegerType::U16, iun1) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));

    // Now, backwards

    // Fixed types fit in implicitly sized int, unless sign mismatch.
    REQUIRE(isAssignable(ii17, &IntegerType::I8) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(ii17, &IntegerType::I16) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(ii17, &IntegerType::U8) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(ii17, &IntegerType::U16) == ConversionResult(ConversionRank::EXACT));
    // `17u` does not fit in an U8
    REQUIRE(isAssignable(iu17, &IntegerType::I8) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    REQUIRE(isAssignable(iu17, &IntegerType::I16) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    REQUIRE(isAssignable(iu17, &IntegerType::U8) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(iu17, &IntegerType::U16) == ConversionResult(ConversionRank::EXACT));

    // Negative destination doesn't matter.
    REQUIRE(isAssignable(iin1, &IntegerType::I8) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(iin1, &IntegerType::I16) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(iin1, &IntegerType::U8) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(iin1, &IntegerType::U16) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(iun1, &IntegerType::I8) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    REQUIRE(isAssignable(iun1, &IntegerType::I16) ==
        ConversionResult(ConversionRank::ERROR, ConversionError::SIGNED_UNSIGNED));
    REQUIRE(isAssignable(iun1, &IntegerType::U8) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(iun1, &IntegerType::U16) == ConversionResult(ConversionRank::EXACT));
  }

  SECTION("Convert float types") {
    REQUIRE(isAssignable(&FloatType::F32, &FloatType::F32)
        == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(&FloatType::F64, &FloatType::F32)
        == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(&FloatType::F32, &FloatType::F64)
        == ConversionResult(ConversionRank::ERROR, ConversionError::PRECISION_LOSS));
    REQUIRE(isAssignable(&FloatType::F32, &IntegerType::I32)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(&FloatType::F64, &IntegerType::I32)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(&FloatType::F32, &BooleanType::BOOL)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(&FloatType::F32, &VoidType::VOID)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
  }

  SECTION("Convert boolean and void types") {
    REQUIRE(isAssignable(&BooleanType::BOOL, &BooleanType::BOOL)
        == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(&VoidType::VOID, &VoidType::VOID)
        == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(&BooleanType::BOOL, &VoidType::VOID)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(&VoidType::VOID, &BooleanType::BOOL)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
  }

  SECTION("Convert class") {
    auto mod = compile(cu,
      "class A {}\n"
      "class B {}\n"
      "interface I {}\n"
      "class C extends A implements I {}\n"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto clsB = cast<TypeDefn>(mod->members()[1])->type();
    auto intI = cast<TypeDefn>(mod->members()[2])->type();
    auto clsC = cast<TypeDefn>(mod->members()[3])->type();
    REQUIRE(isAssignable(clsA, clsA) == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(clsA, clsB)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(clsA, clsC) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(intI, clsC) == ConversionResult(ConversionRank::EXACT));
  }

  SECTION("Structural typing") {
    auto mod = compile(cu,
      "class A { fn x() -> i32 { 0 } }\n"
      "interface I { fn x() -> i32; }\n"
      "interface J { fn x() -> bool; }\n"
      "interface K extends I {}\n"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto intI = cast<TypeDefn>(mod->members()[1])->type();
    auto intJ = cast<TypeDefn>(mod->members()[2])->type();
    auto intK = cast<TypeDefn>(mod->members()[3])->type();
    REQUIRE(isAssignable(intI, clsA) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(intJ, clsA)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(intK, clsA) == ConversionResult(ConversionRank::EXACT));
  }

  SECTION("Convert templated class") {
    auto mod = compile(cu,
      "class A[T] {}\n"
      "class B extends A[i32] {}\n"
      "class C extends A[f32] {}\n"
      "let D: A[i32];"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto clsB = cast<TypeDefn>(mod->members()[1])->type();
    auto clsC = cast<TypeDefn>(mod->members()[2])->type();
    auto letD = cast<ValueDefn>(mod->members()[3])->type();
    REQUIRE(isAssignable(clsA, clsC)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(clsB, clsC)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(clsC, clsB)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(letD, clsB) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(letD, clsC)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
  }

  SECTION("Convert union") {
    auto mod = compile(cu,
      "let A: i32 | f32;\n"
      "let B: i32 | f32 | bool;\n"
      "let C: i32;\n"
    );
    auto letA = cast<ValueDefn>(mod->members()[0])->type();
    auto letB = cast<ValueDefn>(mod->members()[1])->type();
    auto letC = cast<ValueDefn>(mod->members()[2])->type();
    REQUIRE(isAssignable(letA, letB)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(letB, letA) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(letA, letC) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(letC, letA)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(letB, letC) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(letC, letB)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
  }

  SECTION("Convert alias") {
    auto mod = compile(cu,
      "class A {}\n"
      "class B {}\n"
      "interface I {}\n"
      "class C extends A implements I {}\n"
      "type A1 = A;\n"
      "type B1 = B;\n"
      "type C1 = C;\n"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto clsB = cast<TypeDefn>(mod->members()[1])->type();
    auto intI = cast<TypeDefn>(mod->members()[2])->type();
    auto clsC = cast<TypeDefn>(mod->members()[3])->type();
    auto alA1 = cast<TypeDefn>(mod->members()[4])->type();
    auto alB1 = cast<TypeDefn>(mod->members()[5])->type();
    auto alC1 = cast<TypeDefn>(mod->members()[6])->type();
    REQUIRE(isAssignable(clsA, alA1) == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(alA1, clsA) == ConversionResult(ConversionRank::IDENTICAL));
    REQUIRE(isAssignable(alA1, clsB)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(clsA, alB1)
        == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
    REQUIRE(isAssignable(alA1, clsC) == ConversionResult(ConversionRank::EXACT));
    REQUIRE(isAssignable(intI, alC1) == ConversionResult(ConversionRank::EXACT));
  }
}
