#include "catch.hpp"
#include "semamatcher.hpp"
#include "mockreporter.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
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
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    ResolveTypesPass rtPass(cu);
    rtPass.process(mod.get());
    return mod;
  }

  /** Parse a module definition with errors and return the error message. */
  // std::string compileError(CompilationUnit &cu, const char* srcText) {
  //   UseMockReporter umr;
  //   auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
  //   Parser parser(mod->source(), mod->astAlloc());
  //   auto result = parser.module();
  //   mod->setAst(result);
  //   BuildGraphPass bgPass(cu);
  //   bgPass.process(mod.get());
  //   NameResolutionPass nrPass(cu);
  //   nrPass.process(mod.get());
  //   ResolveTypesPass rtPass(cu);
  //   rtPass.process(mod.get());
  //   REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
  //   return MockReporter::INSTANCE.content().str();
  // }
}

TEST_CASE("ResolveTypes", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve primitive type") {
    auto mod = compile(cu, "let x = 0;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::INTEGER);
    REQUIRE_THAT(vdef->type(), TypeEQ("i32"));
  }

  SECTION("Resolve primitive type") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = 1;\n"
        "  result\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }
}
