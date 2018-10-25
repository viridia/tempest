#include "catch.hpp"
#include "semamatcher.hpp"
#include "mockreporter.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/expandspecialization.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "llvm/Support/Casting.h"
#include <iostream>

using namespace tempest::compiler;
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
    diag.reset();
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    CompilationUnit::theCU = &cu;
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    ResolveTypesPass rtPass(cu);
    rtPass.process(mod.get());
    ExpandSpecializationPass esPass(cu);
    esPass.process(mod.get());
    esPass.run();
    CompilationUnit::theCU = nullptr;
    return mod;
  }
}

TEST_CASE("ExpandSpecialization", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Generic function with inferred parameter having two constraints") {
    auto mod = compile(cu,
        "fn x(a0: f32, a1: f64) {\n"
        "  let result = y(a0, a1);\n"
        "}\n"
        "fn y[T](a: T, b: T) => a;\n"
    );
    REQUIRE(cu.symbols().list().size() == 2);
    auto sym = cu.symbols().list()[0];
    REQUIRE(sym->kind == tempest::gen::OutputSym::Kind::FUNCTION);
    sym = cu.symbols().list()[1];
    REQUIRE(sym->kind == tempest::gen::OutputSym::Kind::FUNCTION);
    // REQUIRE_THAT(cu.symbols().list()[0], MemberEQ("fn y[f64]\n"));
  }

  SECTION("Generic multi-arg function with type constraint") {
    auto mod = compile(cu,
        "fn x(pi8: i8, pi16: i16, pi32: i32, pi64: i64) {\n"
        "  let result1 = y(pi8, pi16);\n"
        "  let result1 = y(pi16, pi32);\n"
        "  let result1 = y(pi16, pi64);\n"
        "}\n"
        "fn y[T: i64](a: T, b: T) => a;\n"
        "fn y[T: u64](a: T, b: T) => a;\n"
        "fn y[T: f64](a: T, b: T) => a;\n"
    );
    REQUIRE(cu.symbols().list().size() == 4);
    // REQUIRE_THAT(cu.spec().concreteSpecs()[0], MemberEQ("fn y[i16]\n"));
    // REQUIRE_THAT(cu.spec().concreteSpecs()[1], MemberEQ("fn y[i32]\n"));
    // REQUIRE_THAT(cu.spec().concreteSpecs()[2], MemberEQ("fn y[i64]\n"));
  }

  SECTION("Resolve addition operator") {
    auto mod = compile(cu, "fn x(arg: i32) => arg + 1;\n");
    REQUIRE(cu.symbols().list().size() == 1);
    auto sym = cu.symbols().list()[0];
    REQUIRE(sym->kind == tempest::gen::OutputSym::Kind::FUNCTION);
    // sym = cu.symbols().list()[1];
    // REQUIRE(sym->kind == tempest::gen::OutputSym::Kind::FUNCTION);
    // REQUIRE_THAT(cu.spec().concreteSpecs()[0], MemberEQ("fn infixAdd[i32]\n"));
  }

  SECTION("Generic function that invokes another generic") {
    auto mod = compile(cu,
        "fn x(a0: f32, a1: f64) {\n"
        "  let result = y(a0, a1);\n"
        "}\n"
        "fn y[T](a: T, b: T) => z(a);\n"
        "fn z[T](a: T) => a;\n"
    );
    REQUIRE(cu.symbols().list().size() == 3);
    // REQUIRE_THAT(cu.spec().concreteSpecs()[0], MemberEQ("fn y[f64]\n"));
    // REQUIRE_THAT(cu.spec().concreteSpecs()[1], MemberEQ("fn z[f64]\n"));
  }
}
