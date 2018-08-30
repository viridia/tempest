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
    CompilationUnit::theCU = &cu;
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    ResolveTypesPass rtPass(cu);
    rtPass.process(mod.get());
    CompilationUnit::theCU = nullptr;
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
    CompilationUnit::theCU = nullptr;
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    return MockReporter::INSTANCE.content().str();
  }
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

  SECTION("Resolve local variable type") {
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

  SECTION("Resolve function call type") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y();\n"
        "  result\n"
        "}\n"
        "fn y() {\n"
        "  let result = 1;\n"
        "  result\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Resolve function with parameter") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1);\n"
        "  result\n"
        "}\n"
        "fn y(i: i32) => i;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Resolve function with parameter type error") {
    REQUIRE_THAT(
      compileError(cu,
          "fn x() {\n"
          "  let result = y(1.0);\n"
          "  result\n"
          "}\n"
          "fn y(i: i32) => i;\n"
      ),
      Catch::Contains("cannot convert argument 1 from f64 to i32"));

    REQUIRE_THAT(
      compileError(cu,
          "fn x() {\n"
          "  let result = y(1, 2);\n"
          "  result\n"
          "}\n"
          "fn y(i: i32) => i;\n"
      ),
      Catch::Contains("expects no more than 1 arguments, 2 were supplied"));

    REQUIRE_THAT(
      compileError(cu,
          "fn x() {\n"
          "  let result = y(1);\n"
          "  result\n"
          "}\n"
          "fn y(i: i32, j: i32) => i;\n"
      ),
      Catch::Contains("requires at least 2 arguments, only 1 was supplied"));
  }

  SECTION("Resolve overloaded function with parameter") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1);\n"
        "  result\n"
        "}\n"
        "fn y(i: i32) => i;\n"
        "fn y(i: f32) => i;\n"
        "fn y(i: bool) => i;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Multi-site overloads") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(z(1));\n"
        "  result\n"
        "}\n"
        "fn y(i: i32) => i;\n"
        "fn y(i: f32) => i;\n"
        "fn y(i: bool) => i;\n"
        "fn z(i: i32) => i;\n"
        "fn z(i: f32) => i;\n"
        "fn z(i: bool) => i;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Nested overloads") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(y(y(y(1))));\n"
        "  result\n"
        "}\n"
        "fn y(i: i32) => i;\n"
        "fn y(i: f32) => i;\n"
        "fn y(i: bool) => i;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Multi-argument overloads") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1, true, 1);\n"
        "  result\n"
        "}\n"
        "fn y(a: bool, b: i32, c: i32) => b;\n"
        "fn y(a: i32, b: bool, c: i32) => b;\n"
        "fn y(a: i32, b: i32, c: bool) => b;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("bool"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("bool"));
  }

  SECTION("Specificity overloads") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1, 1);\n"
        "  result\n"
        "}\n"
        "fn y(a: i64, b: i64) => a;\n"
        "fn y(a: i32, b: i32) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }
}
