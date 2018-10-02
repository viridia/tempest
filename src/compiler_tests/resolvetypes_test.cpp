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

  SECTION("Resolve primitive integer constant") {
    auto mod = compile(cu, "let x = 0 + 1;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::INTEGER);
    REQUIRE(vdef->init()->kind == Expr::Kind::INTEGER_LITERAL);
    REQUIRE_THAT(vdef->type(), TypeEQ("i32"));
  }

  SECTION("Resolve primitive float constant") {
    auto mod = compile(cu, "let x = 0. + 1.;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::LET_DEF);
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::FLOAT);
    REQUIRE(vdef->init()->kind == Expr::Kind::FLOAT_LITERAL);
    REQUIRE_THAT(vdef->type(), TypeEQ("f64"));
  }

  SECTION("Implicit cast of integer literal") {
    auto mod = compile(cu,
        "fn x() -> i32 {\n"
        "  0\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    auto body = cast<BlockStmt>(fd->body());
    REQUIRE_THAT(body->type, TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Implicit type promotion of integer variable") {
    auto mod = compile(cu,
        "fn x() -> i32 {\n"
        "  const EEE: i8 = 0;\n"
        "  EEE\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    auto body = cast<BlockStmt>(fd->body());
    REQUIRE(isa<UnaryOp>(body->result));
    auto bodyRes = cast<UnaryOp>(body->result);
    REQUIRE_THAT(bodyRes->type, TypeEQ("i32"));
    REQUIRE_THAT(body->type, TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Implicit cast of integer literal (no block)") {
    auto mod = compile(cu, "fn x() -> i32 => 0;\n");
    auto fd = cast<FunctionDefn>(mod->members().back());
    REQUIRE_THAT(fd->body()->type, TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
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
        "fn y(a: bool, b: bool) => a;\n"
        "fn y(a: f32, b: f32) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
    auto callExpr = cast<ApplyFnOp>(letSt->defn->init());
    auto fnRef = cast<DefnRef>(callExpr->function);
    REQUIRE(fnRef->kind == Expr::Kind::FUNCTION_REF);
    REQUIRE(fnRef->defn->name() == "y");
    auto fn = cast<FunctionDefn>(fnRef->defn);
    REQUIRE_THAT(fn->type(), TypeEQ("fn (i32, i32) -> i32"));
  }

  SECTION("Resolve overloads by return type") {
    auto mod = compile(cu,
        "fn x() -> i32 {\n"
        "  let result: i32 = y();\n"
        "  result\n"
        "}\n"
        "fn y() -> i16 { 0 }\n"
        "fn y() -> i32 { 0 }\n"
        "fn y() -> i64 { 0 }\n"
        "fn y() -> f32 { 0f }\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Ambiguous overloads") {
    REQUIRE_THAT(
      compileError(cu,
          "fn x() {\n"
          "  let result = y(1, 1);\n"
          "  result\n"
          "}\n"
          "fn y(a: i64, b: i32) => a;\n"
          "fn y(a: i32, b: i64) => a;\n"
      ),
      Catch::Contains("Ambiguous overloaded method"));
  }

  SECTION("Varargs assignments") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1, 2, 3, 4);\n"
        "}\n"
        "fn y(a: i32, b: i32...) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    auto callExpr = cast<ApplyFnOp>(letSt->defn->init());
    REQUIRE(callExpr->args.size() == 2);
    REQUIRE_THAT(callExpr, ExprEQ("y(1, [2, 3, 4])"));
  }

  SECTION("Varargs assignments with no args") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1);\n"
        "}\n"
        "fn y(a: i32, b: i32...) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    auto callExpr = cast<ApplyFnOp>(letSt->defn->init());
    REQUIRE(callExpr->args.size() == 2);
    REQUIRE_THAT(callExpr, ExprEQ("y(1, [])"));
  }

  SECTION("Default param values") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1);\n"
        "}\n"
        "fn y(a: i32, b: i32 = 3) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    auto callExpr = cast<ApplyFnOp>(letSt->defn->init());
    auto fnRef = cast<DefnRef>(callExpr->function);
    REQUIRE(callExpr->args.size() == 2);
    REQUIRE_THAT(callExpr, ExprEQ("y(1, 3)"));
    REQUIRE_THAT(fnRef->defn, MemberEQ("fn y\n"));
  }

  SECTION("Generic function with explicit parameter") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y[i32](1);\n"
        "}\n"
        "fn y[T](a: T) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    auto callExpr = cast<ApplyFnOp>(letSt->defn->init());
    auto fnRef = cast<DefnRef>(callExpr->function);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fnRef->defn, MemberEQ("fn y[i32]\n"));
    REQUIRE(fnRef->defn->kind == Member::Kind::SPECIALIZED);
  }

  SECTION("Generic function with inferred parameter") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result = y(1);\n"
        "}\n"
        "fn y[T](a: T) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    auto callExpr = cast<ApplyFnOp>(letSt->defn->init());
    auto fnRef = cast<DefnRef>(callExpr->function);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(fnRef->defn, MemberEQ("fn y[i32]\n"));
    REQUIRE(fnRef->defn->kind == Member::Kind::SPECIALIZED);
  }

  SECTION("Generic function with inferred parameter (2)") {
    auto mod = compile(cu,
        "fn x(arg: f32) {\n"
        "  let result = y(arg);\n"
        "}\n"
        "fn y[T](a: T) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("f32"));
  }

  SECTION("Generic function with inferred parameter having two constraints") {
    auto mod = compile(cu,
        "fn x(a0: f32, a1: f64) {\n"
        "  let result = y(a0, a1);\n"
        "}\n"
        "fn y[T](a: T, b: T) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("f64"));
  }

  SECTION("Generic function with inferred return type") {
    auto mod = compile(cu,
        "fn x() {\n"
        "  let result: i32 = y();\n"
        "}\n"
        "fn y[T]() -> T { const x: T; x }\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("i32"));
  }

  SECTION("Generic function that invokes another generic") {
    auto mod = compile(cu,
        "fn x(a0: f32, a1: f64) {\n"
        "  let result = y(a0, a1);\n"
        "}\n"
        "fn y[T](a: T, b: T) => z(a);\n"
        "fn z[T](a: T) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("f64"));
  }

  SECTION("Return statement") {
    auto mod = compile(cu,
        "fn x() -> i32 {\n"
        "  return y();\n"
        "}\n"
        "fn y[T]() -> T { const x: T; x }\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto retSt = cast<ReturnStmt>(body->stmts[0]);
    REQUIRE_THAT(retSt->expr->type, TypeEQ("i32"));
  }

  SECTION("Generic function with type constraint") {
    auto mod = compile(cu,
        "fn x(arg: f32) {\n"
        "  let result = y(arg);\n"
        "}\n"
        "fn y[T: i8](a: T) => a;\n"
        "fn y[T: f32](a: T) => a;\n"
        "fn y[T: bool](a: T) => a;\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE_THAT(letSt->defn->type(), TypeEQ("f32"));
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
    auto fd = cast<FunctionDefn>(mod->members().front());
    auto body = cast<BlockStmt>(fd->body());
    REQUIRE_THAT(cast<LocalVarStmt>(body->stmts[0])->defn->type(), TypeEQ("i16"));
    REQUIRE_THAT(cast<LocalVarStmt>(body->stmts[1])->defn->type(), TypeEQ("i32"));
    REQUIRE_THAT(cast<LocalVarStmt>(body->stmts[2])->defn->type(), TypeEQ("i64"));
  }

  SECTION("Type constraint failure") {
    REQUIRE_THAT(
      compileError(cu,
          "fn x(arg: f32) {\n"
          "  let result = y(arg);\n"
          "}\n"
          "fn y[T: i8](a: T) => a;\n"
          "fn y[T: bool](a: T) => a;\n"
      ),
      Catch::Contains("T cannot be bound to type f32, it must be a subtype of i8"));
  }

  SECTION("Resolve addition operator") {
    auto mod = compile(cu, "fn x(arg: i32) => arg + 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Resolve addition operator (large int") {
    auto mod = compile(cu, "fn x(arg: i32) => arg + 0x100000000;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i64"));
  }

  SECTION("Resolve addition operator (constant folding") {
    auto mod = compile(cu, "fn x() => 100000001 - 100000000;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("Resolve addition operator (constant folding - unsigned") {
    auto mod = compile(cu, "fn x() => 100000001 - 100000000u;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("u32"));
  }

  SECTION("Resolve addition operator (float)") {
    auto mod = compile(cu, "fn x(arg: f32) => arg + 1f;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("f32"));
  }

  SECTION("Resolve subtraction operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg + 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("u32"));
  }

  SECTION("Resolve subtraction operator (unsigned)") {
    auto mod = compile(cu, "fn x(arg: i32) => arg + 1u;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("u32"));
  }

  SECTION("Resolve multiply operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg * 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("u32"));
  }

  SECTION("Resolve divide operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg / 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("u32"));
  }

  SECTION("Resolve remainder operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg % 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("u32"));
  }

  SECTION("Resolve less-than operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg < 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("bool"));
  }

  SECTION("Resolve less-than-or-equal operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg <= 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("bool"));
  }

  SECTION("Resolve greater-than operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg > 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("bool"));
  }

  SECTION("Resolve greater-than-or-equal operator") {
    auto mod = compile(cu, "fn x(arg: u32) => arg >= 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("bool"));
  }

  SECTION("Conflicting types for generic parameter") {
    REQUIRE_THAT(
      compileError(cu,
          "fn x(a0: f32, a1: i32) => a0 + a1;\n"
      ),
      Catch::Contains("subtype of i64"));
  }

  SECTION("If statement - disjoint returns") {
    auto mod = compile(cu,
        "fn x(a: i32) {\n"
        "  if a {\n"
        "    a\n"
        "  } else {\n"
        "    false\n"
        "  }\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("bool | i32"));
  }

  SECTION("If statement - overlapping returns") {
    auto mod = compile(cu,
        "fn x(a: i32, b: i16) {\n"
        "  if a {\n"
        "    a\n"
        "  } else {\n"
        "    b\n"
        "  }\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("i32"));
  }

  SECTION("If statement - no else") {
    auto mod = compile(cu,
        "fn x(a: i32, b: i16) {\n"
        "  if a {\n"
        "    a\n"
        "  }\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("void | i32"));
  }

  SECTION("If statement - disjoint return statements") {
    auto mod = compile(cu,
        "fn x(a: i32) {\n"
        "  if a {\n"
        "    return a\n"
        "  } else {\n"
        "    return false;\n"
        "  }\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("bool | i32"));
  }

  SECTION("While statement") {
    auto mod = compile(cu,
        "fn x(a: i32) {\n"
        "  while a {\n"
        "    a\n"
        "  }\n"
        "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    REQUIRE_THAT(fd->type()->returnType, TypeEQ("void"));
  }
}
