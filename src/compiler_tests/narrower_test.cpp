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

TEST_CASE("Convert.Narrower", "[sema]") {
  CompilationUnit cu;

  SECTION("Compare integer types") {
    REQUIRE(isEqualOrNarrower(&IntegerType::I8, &IntegerType::I8));
    REQUIRE(isEqualOrNarrower(&IntegerType::I32, &IntegerType::I32));
    REQUIRE_FALSE(isEqualOrNarrower(&IntegerType::I16, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(&IntegerType::U16, &IntegerType::I16));
    REQUIRE(isEqualOrNarrower(&IntegerType::I8, &IntegerType::I16));
    REQUIRE_FALSE(isEqualOrNarrower(&FloatType::F32, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(&BooleanType::BOOL, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(&VoidType::VOID, &IntegerType::I8));
  }

  SECTION("Compare implicit integer types") {
    llvm::APInt seventeen(64, 17);
    llvm::APInt oneThousand(64, 1024);
    llvm::APInt negativeOne(64, -1, true);
    auto ii17 = cu.types().createIntegerType(seventeen, false); // A 6-bit number (signed)
    auto iu17 = cu.types().createIntegerType(seventeen, true); // A 5-bit number (unsigned)
    auto ii1k = cu.types().createIntegerType(oneThousand, false); // A 12-bit number (signed)
    auto iu1k = cu.types().createIntegerType(oneThousand, true); // An 11-bit number (unsigned)
    auto iin1 = cu.types().createIntegerType(negativeOne, false); // A 2-bit number (signed)
    auto iun1 = cu.types().createIntegerType(negativeOne, true); // An 1-bit number (unsigned)

    REQUIRE(isEqualOrNarrower(&IntegerType::I8, ii17));
    REQUIRE(isEqualOrNarrower(&IntegerType::I8, iu17));
    REQUIRE(isEqualOrNarrower(&IntegerType::I8, ii1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::I8, iu1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::I8, iin1));
    REQUIRE(isEqualOrNarrower(&IntegerType::I8, iun1));

    REQUIRE_FALSE(isEqualOrNarrower(ii17, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(iu17, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(ii1k, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(iu1k, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(iin1, &IntegerType::I8));
    REQUIRE_FALSE(isEqualOrNarrower(iun1, &IntegerType::I8));

    REQUIRE(isEqualOrNarrower(&IntegerType::U8, ii17));
    REQUIRE(isEqualOrNarrower(&IntegerType::U8, iu17));
    REQUIRE(isEqualOrNarrower(&IntegerType::U8, ii1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::U8, iu1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::U8, iin1));
    REQUIRE(isEqualOrNarrower(&IntegerType::U8, iun1));

    REQUIRE_FALSE(isEqualOrNarrower(ii17, &IntegerType::U8));
    REQUIRE_FALSE(isEqualOrNarrower(iu17, &IntegerType::U8));
    REQUIRE_FALSE(isEqualOrNarrower(ii1k, &IntegerType::U8));
    REQUIRE_FALSE(isEqualOrNarrower(iu1k, &IntegerType::U8));
    REQUIRE_FALSE(isEqualOrNarrower(iin1, &IntegerType::U8));
    REQUIRE_FALSE(isEqualOrNarrower(iun1, &IntegerType::U8));

    REQUIRE(isEqualOrNarrower(&IntegerType::I16, ii17));
    REQUIRE(isEqualOrNarrower(&IntegerType::I16, iu17));
    REQUIRE(isEqualOrNarrower(&IntegerType::I16, ii1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::I16, iu1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::I16, iin1));
    REQUIRE(isEqualOrNarrower(&IntegerType::I16, iun1));

    REQUIRE_FALSE(isEqualOrNarrower(ii17, &IntegerType::I16));
    REQUIRE_FALSE(isEqualOrNarrower(iu17, &IntegerType::I16));
    REQUIRE_FALSE(isEqualOrNarrower(ii1k, &IntegerType::I16));
    REQUIRE_FALSE(isEqualOrNarrower(iu1k, &IntegerType::I16));
    REQUIRE_FALSE(isEqualOrNarrower(iin1, &IntegerType::I16));
    REQUIRE_FALSE(isEqualOrNarrower(iun1, &IntegerType::I16));

    REQUIRE(isEqualOrNarrower(&IntegerType::U16, ii17));
    REQUIRE(isEqualOrNarrower(&IntegerType::U16, iu17));
    REQUIRE(isEqualOrNarrower(&IntegerType::U16, ii1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::U16, iu1k));
    REQUIRE(isEqualOrNarrower(&IntegerType::U16, iin1));
    REQUIRE(isEqualOrNarrower(&IntegerType::U16, iun1));

    REQUIRE_FALSE(isEqualOrNarrower(ii17, &IntegerType::U16));
    REQUIRE_FALSE(isEqualOrNarrower(iu17, &IntegerType::U16));
    REQUIRE_FALSE(isEqualOrNarrower(ii1k, &IntegerType::U16));
    REQUIRE_FALSE(isEqualOrNarrower(iu1k, &IntegerType::U16));
    REQUIRE_FALSE(isEqualOrNarrower(iin1, &IntegerType::U16));
    REQUIRE_FALSE(isEqualOrNarrower(iun1, &IntegerType::U16));
  }

  SECTION("Compare float types") {
    REQUIRE(isEqualOrNarrower(&FloatType::F32, &FloatType::F32));
    REQUIRE(isEqualOrNarrower(&FloatType::F32, &FloatType::F64));
    REQUIRE_FALSE(isEqualOrNarrower(&FloatType::F64, &FloatType::F32));
    REQUIRE_FALSE(isEqualOrNarrower(&IntegerType::I32, &FloatType::F32));
    REQUIRE_FALSE(isEqualOrNarrower(&IntegerType::I32, &FloatType::F64));
    REQUIRE_FALSE(isEqualOrNarrower(&BooleanType::BOOL, &FloatType::F32));
    REQUIRE_FALSE(isEqualOrNarrower(&VoidType::VOID, &FloatType::F32));
  }

  SECTION("Compare boolean and void types") {
    REQUIRE(isEqualOrNarrower(&BooleanType::BOOL, &BooleanType::BOOL));
    REQUIRE(isEqualOrNarrower(&VoidType::VOID, &VoidType::VOID));
    REQUIRE_FALSE(isEqualOrNarrower(&VoidType::VOID, &BooleanType::BOOL));
    REQUIRE_FALSE(isEqualOrNarrower(&BooleanType::BOOL, &VoidType::VOID));
  }

  SECTION("Compare class") {
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
    REQUIRE(isEqualOrNarrower(clsA, clsA));
    REQUIRE_FALSE(isEqualOrNarrower(clsB, clsA));
    REQUIRE_FALSE(isEqualOrNarrower(clsA, clsB));
    REQUIRE(isEqualOrNarrower(clsC, clsA));
    REQUIRE(isEqualOrNarrower(clsC, intI));
  }

  SECTION("Structural typing") {
    auto mod = compile(cu,
      "class A { x() -> i32 { 0 } }\n"
      "interface I { x() -> i32; }\n"
      "interface J { x() -> bool; }\n"
      "interface K extends I {}\n"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto intI = cast<TypeDefn>(mod->members()[1])->type();
    auto intJ = cast<TypeDefn>(mod->members()[2])->type();
    auto intK = cast<TypeDefn>(mod->members()[3])->type();
    REQUIRE(isEqualOrNarrower(clsA, intI));
    REQUIRE_FALSE(isEqualOrNarrower(clsA, intJ));
    REQUIRE(isEqualOrNarrower(clsA, intK));
  }

  SECTION("Compare templated class") {
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
    REQUIRE_FALSE(isEqualOrNarrower(clsC, clsA));
    REQUIRE_FALSE(isEqualOrNarrower(clsC, clsB));
    REQUIRE_FALSE(isEqualOrNarrower(clsB, clsC));
    REQUIRE_FALSE(isEqualOrNarrower(letD, clsB));
    REQUIRE(isEqualOrNarrower(clsB, letD));
    REQUIRE_FALSE(isEqualOrNarrower(clsC, letD));
  }

  SECTION("Compare union") {
    auto mod = compile(cu,
      "let A: i32 | f32;\n"
      "let B: i32 | f32 | bool;\n"
      "let C: i32;\n"
    );
    auto letA = cast<ValueDefn>(mod->members()[0])->type();
    auto letB = cast<ValueDefn>(mod->members()[1])->type();
    auto letC = cast<ValueDefn>(mod->members()[2])->type();
    REQUIRE_FALSE(isEqualOrNarrower(letB, letA));
    REQUIRE(isEqualOrNarrower(letA, letB));
    REQUIRE(isEqualOrNarrower(letC, letA));
    REQUIRE_FALSE(isEqualOrNarrower(letA, letC));
    REQUIRE(isEqualOrNarrower(letC, letB));
    REQUIRE_FALSE(isEqualOrNarrower(letB, letC));
  }
}
