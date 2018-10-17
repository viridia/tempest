#include "catch.hpp"
#include "semamatcher.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include <iostream>

using namespace tempest::compiler;
using namespace tempest::sema::graph;
using namespace tempest::sema::pass;
using namespace std;
using tempest::parse::Parser;
using tempest::source::Location;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

namespace {
  /** Parse a module definition and apply buildgraph pass. */
  std::unique_ptr<Module> parseModule(const char* srcText) {
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    auto result = parser.module();
    mod->setAst(result);
    CompilationUnit cu;
    BuildGraphPass pass(cu);
    pass.process(mod.get());
    return mod;
  }
}

TEST_CASE("BuildGraph", "[sema]") {
  const Location L;

  SECTION("TypeDefn") {
    auto mod = parseModule(
        "class X {}\n"
        "struct X {}\n"
        "trait X {}\n"
        "interface X {}\n"
        "type Optional[T] = T | void;\n"
    );
    REQUIRE_THAT(
      mod.get(),
      MemberEQ(
        "module test.mod {\n"
        "  class X {\n"
        "  }\n"
        "  struct X {\n"
        "  }\n"
        "  trait X {\n"
        "  }\n"
        "  interface X {\n"
        "  }\n"
        "  type Optional {\n"
        "  }\n"
      "}\n"
      ));
  }

  SECTION("TypeDefn member") {
    auto mod = parseModule(
      "class X {\n"
      "  x: Point;\n"
      "  private y: i32;\n"
      "  final const z: [bool];\n"
      "}\n"
    );
    REQUIRE_THAT(
      mod.get(),
      MemberEQ(
        "module test.mod {\n"
        "  class X {\n"
        "    x;\n"
        "    private y;\n"
        "    const z;\n"
        "  }\n"
        "}\n"
      ));
  }
}
