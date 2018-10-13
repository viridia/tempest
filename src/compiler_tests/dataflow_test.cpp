#include "catch.hpp"
#include "semamatcher.hpp"
#include "mockreporter.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/dataflow.hpp"
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
    auto prevErrorCount = diag.errorCount();
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
    DataFlowPass dfPass(cu);
    dfPass.process(mod.get());
    CompilationUnit::theCU = nullptr;
    REQUIRE(diag.errorCount() == prevErrorCount);
    return mod;
  }

  /** Parse a module definition with errors and return the error message. */
  std::string compileError(CompilationUnit &cu, const char* srcText) {
    UseMockReporter umr;
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
    DataFlowPass dfPass(cu);
    dfPass.process(mod.get());
    CompilationUnit::theCU = nullptr;
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    return MockReporter::INSTANCE.content().str();
  }
}

TEST_CASE("DataFlow", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("initialized variable") {
    compile(cu,
      "class A {\n"
      "  x: i32;\n"
      "  fn new() {\n"
      "    let y: i32 = 0;\n"
      "    x = y;\n"
      "  }\n"
      "}\n"
    );
  }

  SECTION("assigned variable") {
    compile(cu,
      "class A {\n"
      "  x: i32;\n"
      "  fn new() {\n"
      "    let y: i32;\n"
      "    y = 0;\n"
      "    x = y;\n"
      "  }\n"
      "}\n"
    );
  }

  SECTION("unused variable") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  x: i32;\n"
          "  fn new() {\n"
          "    let x: i32 = 0;\n"
          "  }\n"
          "}\n"
      ),
      Catch::Contains("Unused local variable"));
  }

  SECTION("uninitialized variable") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  x: i32;\n"
          "  fn new() {\n"
          "    let y: i32;\n"
          "    x = y;\n"
          "  }\n"
          "}\n"
      ),
      Catch::Contains("uninitialized variable"));
  }

  SECTION("uninitialized field") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  x: i32;\n"
          "  fn new() {}\n"
          "}\n"
      ),
      Catch::Contains("Field not initialized"));
  }

  SECTION("base class with no default constructor") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  fn new(x: i32) {}\n"
          "}\n"
          "class B extends A {\n"
          "  fn new() {}\n"
          "}\n"
      ),
      Catch::Contains("Base class has no default constructor"));
  }

  SECTION("auto-initialized variable") {
    auto mod = compile(cu,
      "class A {\n"
      "  x: i32 = 0;\n"
      "  fn new() {}\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    auto fd = cast<FunctionDefn>(td->members().back());
    auto blk = cast<BlockStmt>(fd->body());
    REQUIRE(blk->stmts.size() == 2);
    REQUIRE(blk->stmts[0]->kind == Expr::Kind::CALL_SUPER);
    REQUIRE(blk->stmts[1]->kind == Expr::Kind::ASSIGN);
  }

  SECTION("implicit call to super of templated base class") {
    auto mod = compile(cu,
      "class A[T] {\n"
      "  fn new() {}\n"
      "}\n"
      "class B extends A[i32] {\n"
      "  fn new() {}\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    auto fd = cast<FunctionDefn>(td->members().back());
    auto blk = cast<BlockStmt>(fd->body());
    REQUIRE(blk->stmts.size() == 1);
    REQUIRE(blk->stmts[0]->kind == Expr::Kind::CALL_SUPER);
    auto call = cast<ApplyFnOp>(blk->stmts[0]);
    auto callable = cast<DefnRef>(call->function);
    REQUIRE(callable->defn->kind == Defn::Kind::SPECIALIZED);
  }
}
