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
#include "tempest/sema/pass/findoverrides.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "llvm/Support/Casting.h"
#include <iostream>

using namespace tempest::compiler;
using namespace tempest::sema::graph;
using namespace tempest::sema::pass;
using namespace tempest::gen;
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
    FindOverridesPass foPass(cu);
    foPass.process(mod.get());
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

  SECTION("Trait requirements") {
    auto mod = compile(cu,
      "trait X {\n"
      "  f(a0: f32, a1: bool) -> i32;\n"
      "}\n"
      "class A implements X {\n"
      "  f(a0: f32, a1: bool) -> i32 { return 7; }\n"
      "}\n"
      "class B[T: X] {\n"
      "  b(a: T) -> i32 { return a.f(1f, false); }\n"
      "}\n"
      "fn test() {\n"
      "  const a = A();\n"
      "  const b = B[A]();\n"
      "  const c = b.b(a);\n"
      "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    (void) fd;
    // REQUIRE_THAT(vd->type(), TypeEQ("i32"));
    // REQUIRE_THAT(vd->init(), ExprEQ("x.f()"));
  }

  SECTION("Implement interface") {
    auto mod = compile(cu,
      "interface X {\n"
      "  f(a0: f32, a1: bool) -> i32;\n"
      "}\n"
      "class A implements X {\n"
      "  f(a0: f32, a1: bool) -> i32 { return 7; }\n"
      "}\n"
    );
    REQUIRE(cu.symbols().list().size() == 5);

    REQUIRE(cu.symbols().list()[0]->kind == OutputSym::Kind::IFACE_DESC);
    auto if0 = cast<InterfaceDescriptorSym>(cu.symbols().list()[0]);
    REQUIRE(if0->typeDefn->name() == "X");

    REQUIRE(cu.symbols().list()[1]->kind == OutputSym::Kind::CLS_DESC);
    auto cl1 = cast<ClassDescriptorSym>(cu.symbols().list()[1]);
    REQUIRE(cl1->typeDefn->name() == "A");
    REQUIRE(cl1->methodTable.size() == 1);
    REQUIRE(cl1->methodTable[0]->function->name() == "f");
    REQUIRE(cl1->methodTable[0]->typeArgs.size() == 0);
    REQUIRE(cl1->interfaceTable.size() == 1);
    REQUIRE(cl1->interfaceTable[0]->iface->typeDefn->name() == "X");
    REQUIRE(cl1->interfaceTable[0]->iface->typeArgs.size() == 0);

    REQUIRE(cu.symbols().list()[2]->kind == OutputSym::Kind::CLS_DESC);
    auto cl2 = cast<ClassDescriptorSym>(cu.symbols().list()[2]);
    REQUIRE(cl2->typeDefn->name() == "Object");
    REQUIRE(cl2->methodTable.size() == 0);

    REQUIRE(cu.symbols().list()[3]->kind == OutputSym::Kind::FUNCTION);
    auto fd3 = cast<FunctionSym>(cu.symbols().list()[3]);
    REQUIRE(fd3->function->name() == "f");

    REQUIRE(cu.symbols().list()[4]->kind == OutputSym::Kind::CLS_IFACE_TRANS);
    auto ci4 = cast<ClassInterfaceTranslationSym>(cu.symbols().list()[4]);
    REQUIRE(ci4->methodTable.size() == 1);
    REQUIRE(ci4->methodTable[0]->function->name() == "f");
    REQUIRE(ci4->methodTable[0]->typeArgs.size() == 0);
  }

  SECTION("Implement interface with params") {
    auto mod = compile(cu,
      "interface X[T] {\n"
      "  f(a0: T, a1: bool) -> i32;\n"
      "}\n"
      "class A implements X[f32] {\n"
      "  f(a0: f32, a1: bool) -> i32 { return 7; }\n"
      "}\n"
    );
    REQUIRE(cu.symbols().list().size() == 5);

    REQUIRE(cu.symbols().list()[0]->kind == OutputSym::Kind::CLS_DESC);
    auto cl0 = cast<ClassDescriptorSym>(cu.symbols().list()[0]);
    REQUIRE(cl0->typeDefn->name() == "A");
    REQUIRE(cl0->methodTable.size() == 1);
    REQUIRE(cl0->methodTable[0]->function->name() == "f");
    REQUIRE(cl0->methodTable[0]->typeArgs.size() == 0);
    REQUIRE(cl0->interfaceTable.size() == 1);
    REQUIRE(cl0->interfaceTable[0]->iface->typeDefn->name() == "X");
    REQUIRE(cl0->interfaceTable[0]->iface->typeArgs.size() == 1);
    REQUIRE_THAT(cl0->interfaceTable[0]->iface->typeArgs[0], TypeEQ("f32"));

    REQUIRE(cu.symbols().list()[1]->kind == OutputSym::Kind::CLS_DESC);
    auto cl1 = cast<ClassDescriptorSym>(cu.symbols().list()[1]);
    REQUIRE(cl1->typeDefn->name() == "Object");
    REQUIRE(cl1->methodTable.size() == 0);

    REQUIRE(cu.symbols().list()[2]->kind == OutputSym::Kind::FUNCTION);
    auto fd2 = cast<FunctionSym>(cu.symbols().list()[2]);
    REQUIRE(fd2->function->name() == "f");

    REQUIRE(cu.symbols().list()[3]->kind == OutputSym::Kind::IFACE_DESC);
    auto if3 = cast<InterfaceDescriptorSym>(cu.symbols().list()[3]);
    REQUIRE(if3->typeDefn->name() == "X");
    REQUIRE(if3->typeArgs.size() == 1);

    REQUIRE(cu.symbols().list()[4]->kind == OutputSym::Kind::CLS_IFACE_TRANS);
    auto ci4 = cast<ClassInterfaceTranslationSym>(cu.symbols().list()[4]);
    REQUIRE(ci4->methodTable.size() == 1);
    REQUIRE(ci4->methodTable[0]->function->name() == "f");
    REQUIRE(ci4->methodTable[0]->typeArgs.size() == 0);
  }

  SECTION("Implement abstract base class") {
    auto mod = compile(cu,
      "abstract class X {\n"
      "  abstract f(a0: f32, a1: bool) -> i32;\n"
      "}\n"
      "class A extends X {\n"
      "  override f(a0: f32, a1: bool) -> i32 { return 7; }\n"
      "}\n"
    );
    REQUIRE(cu.symbols().list().size() == 4);
    REQUIRE(cu.symbols().list()[0]->kind == OutputSym::Kind::CLS_DESC);
    REQUIRE(cu.symbols().list()[1]->kind == OutputSym::Kind::CLS_DESC);
    REQUIRE(cu.symbols().list()[2]->kind == OutputSym::Kind::CLS_DESC);
    REQUIRE(cu.symbols().list()[3]->kind == OutputSym::Kind::FUNCTION);
  }

  SECTION("Implement concrete base class") {
    auto mod = compile(cu,
      "class X[T] {\n"
      "  f(a0: T, a1: bool) -> i32 { 0 }\n"
      "}\n"
      "class A extends X[f32] {\n"
      "  g(a0: f32, a1: bool) -> i32 { return 7; }\n"
      "}\n"
    );
    REQUIRE(cu.symbols().list().size() == 5);
    REQUIRE(cu.symbols().list()[0]->kind == OutputSym::Kind::CLS_DESC);
    REQUIRE(cu.symbols().list()[1]->kind == OutputSym::Kind::CLS_DESC);
    REQUIRE(cu.symbols().list()[2]->kind == OutputSym::Kind::FUNCTION);
    auto fd2 = cast<FunctionSym>(cu.symbols().list()[2]);
    REQUIRE(fd2->function->name() == "f");
    REQUIRE(fd2->typeArgs.size() == 1);
    REQUIRE_THAT(fd2->typeArgs[0], TypeEQ("f32"));

    REQUIRE(cu.symbols().list()[3]->kind == OutputSym::Kind::FUNCTION);
    auto fd3 = cast<FunctionSym>(cu.symbols().list()[3]);
    REQUIRE(fd3->function->name() == "g");
  }
}
