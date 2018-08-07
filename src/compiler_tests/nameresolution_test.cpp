#include "catch.hpp"
#include "semamatcher.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
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

TEST_CASE("NameResolution", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve primitive type") {
    auto mod = compile(cu, "let X: i32;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::INTEGER);
    REQUIRE_THAT(vdef->type(), TypeEQ("i32"));
  }

  SECTION("Resolve union type") {
    auto mod = compile(cu, "let X: i32 | f32 | bool;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::UNION);
    REQUIRE(cast<UnionType>(vdef->type())->members.size() == 3);
    REQUIRE_THAT(vdef->type(), TypeEQ("bool | i32 | f32"));
  }

  SECTION("Resolve union of singletons") {
    auto mod = compile(cu, "let X: 1 | 2 | 3;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::UNION);
    REQUIRE(cast<UnionType>(vdef->type())->members.size() == 3);
    REQUIRE_THAT(vdef->type(), TypeEQ("1 | 2 | 3"));
  }

  SECTION("Resolve tuple type") {
    auto mod = compile(cu, "let X: (i32, f32, bool);");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::TUPLE);
    REQUIRE(cast<TupleType>(vdef->type())->members.size() == 3);
    REQUIRE_THAT(vdef->type(), TypeEQ("(i32, f32, bool)"));
  }

  SECTION("Resolve class type") {
    auto mod = compile(cu,
      "class A {}\n"
      "let X: A;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class A"));
  }

  SECTION("Resolve class member explicitly") {
    auto mod = compile(cu,
      "class A {\n"
      "  class B {}\n"
      "}\n"
      "let X: A.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Resolve class member from within class") {
    auto mod = compile(cu,
      "class A {\n"
      "  class B {}\n"
      "  const X: B;\n"
      "}"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->kind == Defn::Kind::TYPE);
    auto vdef = cast<ValueDefn>(td->members().back());
    REQUIRE(vdef->kind == Defn::Kind::CONST_DEF);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Resolve inherited class member") {
    auto mod = compile(cu,
      "class A {\n"
      "  class B {}\n"
      "}\n"
      "class C extends A {}\n"
      "let X: C.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Resolve inherited specialized class member") {
    auto mod = compile(cu,
      "class A[T] {\n"
      "  class B {}\n"
      "}\n"
      "class C extends A[i32] {}\n"
      "let X: C.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::SPECIALIZED);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B[i32]"));
  }

  // SECTION("Resolve alias type") {
  //   auto mod = compile(cu,
  //     "type A = i32 | void;\n"
  //     "let X: A;"
  //   );
  //   auto vdef = cast<ValueDefn>(mod->members().back());
  //   REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
  //   REQUIRE(vdef->type() != nullptr);
  //   REQUIRE(vdef->type()->kind == Type::Kind::ALIAS);
  //   REQUIRE_THAT(vdef->type(), TypeEQ("class A"));
  // }

  // Enum type
  //
}
