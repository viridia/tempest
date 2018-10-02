#include "catch.hpp"
#include "mockreporter.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/eval/evalconst.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "llvm/ADT/SmallString.h"

using namespace tempest::compiler;
using namespace tempest::sema::eval;
using namespace tempest::sema::graph;
using namespace tempest::sema::pass;
using tempest::source::Location;
using tempest::parse::Parser;

namespace llvm {
  ::std::ostream& operator<<(::std::ostream& os, const llvm::APInt& value) {
    llvm::SmallString<16> str;
    value.toString(str, 10, true);
    os << str;
    return os;
  }
}

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
    CompilationUnit::theCU = nullptr;
    return mod;
  }
}

TEST_CASE("EvalConst.Integer", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Addition") {
    auto mod = compile(cu, "fn x() => 1 + 1;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 2));
  }

  SECTION("Subtraction") {
    auto mod = compile(cu, "fn x() => 7 - 5;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 2));
  }

  SECTION("Multiplication") {
    auto mod = compile(cu, "fn x() => 5 * 3;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 15));
  }

  SECTION("Division") {
    auto mod = compile(cu, "fn x() => 7 / 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 3));
  }

  SECTION("Remainder") {
    auto mod = compile(cu, "fn x() => 7 % 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 1));
  }

  SECTION("BitOr") {
    auto mod = compile(cu, "fn x() => 5 | 3;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 7));
  }

  SECTION("BitAnd") {
    auto mod = compile(cu, "fn x() => 5 & 3;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 1));
  }

  SECTION("BitXor") {
    auto mod = compile(cu, "fn x() => 5 ^ 3;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 6));
  }

  SECTION("RShift") {
    auto mod = compile(cu, "fn x() => 15 >> 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 3));
  }

  SECTION("LShift") {
    auto mod = compile(cu, "fn x() => 15 << 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult.sext(64) == llvm::APInt(64, 60));
  }

  SECTION("EQ") {
    auto mod = compile(cu, "fn x() => 15 == 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 15 == 15;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);
  }

  SECTION("NE") {
    auto mod = compile(cu, "fn x() => 15 != 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 15 != 15;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);
  }

  SECTION("LT") {
    auto mod = compile(cu, "fn x() => 1 < 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 2 < 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 3 < 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);
  }

  SECTION("LE") {
    auto mod = compile(cu, "fn x() => 1 <= 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 2 <= 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 3 <= 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);
  }

  SECTION("GT") {
    auto mod = compile(cu, "fn x() => 1 > 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 2 > 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 3 > 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);
  }

  SECTION("LE") {
    auto mod = compile(cu, "fn x() => 1 >= 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 2 >= 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 3 >= 2;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);
  }
}

TEST_CASE("EvalConst.Integer.Unsigned", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Addition") {
    auto mod = compile(cu, "fn x() => 1 + 1u;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 2));
  }

  SECTION("Subtraction") {
    auto mod = compile(cu, "fn x() => 7 - 5u;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 2));
  }

  SECTION("Multiplication") {
    auto mod = compile(cu, "fn x() => 5u * 3;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 15));
  }

  SECTION("Division") {
    auto mod = compile(cu, "fn x() => 7u / 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 3));
  }

  SECTION("Remainder") {
    auto mod = compile(cu, "fn x() => 7 % 2u;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 1));
  }

  SECTION("RShift") {
    auto mod = compile(cu, "fn x() => 15u >> 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 3));

    mod = compile(cu, "fn x() => -15 >> 2u;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE_FALSE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, -4));
  }

  SECTION("LShift") {
    auto mod = compile(cu, "fn x() => 15u << 2;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::INT);
    REQUIRE(result.isUnsigned);
    REQUIRE_FALSE(result.hasSize);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.intResult == llvm::APInt(64, 60));
  }

  SECTION("LT") {
    auto mod = compile(cu, "fn x() => 1 < 2u;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 2 < 2u;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 3 < 2u;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);
  }
}

TEST_CASE("EvalConst.Float", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Constants") {
    auto mod = compile(cu, "fn x() => 1.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::FLOAT);
    REQUIRE(result.size == EvalResult::F64);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.floatResult.isExactlyValue(1.));

    mod = compile(cu, "fn x() => 1f;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::FLOAT);
    REQUIRE(result.size == EvalResult::F32);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.floatResult.isExactlyValue(1.));
  }

  SECTION("Addition") {
    auto mod = compile(cu, "fn x() => 1. + 1f;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::FLOAT);
    REQUIRE(result.size == EvalResult::F64);
    REQUIRE_FALSE(result.error);
    REQUIRE_THAT(result.floatResult.convertToDouble(), Catch::WithinAbs(2., 0.01));
  }

  SECTION("Subtraction") {
    auto mod = compile(cu, "fn x() => 7f - 5f;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::FLOAT);
    REQUIRE(result.size == EvalResult::F32);
    REQUIRE_FALSE(result.error);
    REQUIRE_THAT(result.floatResult.convertToFloat(), Catch::WithinAbs(2., 0.01));
  }

  SECTION("Multiplication") {
    auto mod = compile(cu, "fn x() => 5. * 3.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::FLOAT);
    REQUIRE(result.size == EvalResult::F64);
    REQUIRE_FALSE(result.error);
    REQUIRE_THAT(result.floatResult.convertToDouble(), Catch::WithinAbs(15., 0.01));
  }

  SECTION("Division") {
    auto mod = compile(cu, "fn x() => 7. / 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::FLOAT);
    REQUIRE(result.size == EvalResult::F64);
    REQUIRE_FALSE(result.error);
    REQUIRE_THAT(result.floatResult.convertToDouble(), Catch::WithinAbs(3.5, 0.01));
  }

  SECTION("Division by zero") {
    auto mod = compile(cu, "fn x() => 7. / 0.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    UseMockReporter umr;
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    REQUIRE(result.error);
    REQUIRE_THAT(
        MockReporter::INSTANCE.content().str(),
        Catch::Contains("divide by zero"));
  }

  SECTION("Remainder") {
    auto mod = compile(cu, "fn x() => 7. % 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::FLOAT);
    REQUIRE(result.size == EvalResult::F64);
    REQUIRE_FALSE(result.error);
    REQUIRE_THAT(result.floatResult.convertToDouble(), Catch::WithinAbs(1., 0.01));
  }

  SECTION("BitOr") {
    auto mod = compile(cu, "fn x() => 5. | 3.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    UseMockReporter umr;
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    REQUIRE(result.error);
    REQUIRE_THAT(
        MockReporter::INSTANCE.content().str(),
        Catch::Contains("Invalid operation for floating-point operands"));
  }

  SECTION("BitAnd") {
    auto mod = compile(cu, "fn x() => 5. & 3.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    UseMockReporter umr;
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    REQUIRE(result.error);
    REQUIRE_THAT(
        MockReporter::INSTANCE.content().str(),
        Catch::Contains("Invalid operation for floating-point operands"));
  }

  SECTION("BitXor") {
    auto mod = compile(cu, "fn x() => 5. ^ 3.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    UseMockReporter umr;
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    REQUIRE(result.error);
    REQUIRE_THAT(
        MockReporter::INSTANCE.content().str(),
        Catch::Contains("Invalid operation for floating-point operands"));
  }

  SECTION("RShift") {
    auto mod = compile(cu, "fn x() => 15. >> 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    UseMockReporter umr;
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    REQUIRE(result.error);
    REQUIRE_THAT(
        MockReporter::INSTANCE.content().str(),
        Catch::Contains("Invalid operation for floating-point operands"));
  }

  SECTION("LShift") {
    auto mod = compile(cu, "fn x() => 15. << 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    UseMockReporter umr;
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    REQUIRE(result.error);
    REQUIRE_THAT(
        MockReporter::INSTANCE.content().str(),
        Catch::Contains("Invalid operation for floating-point operands"));
  }

  SECTION("EQ") {
    auto mod = compile(cu, "fn x() => 15. == 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 15. == 15.;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);
  }

  SECTION("NE") {
    auto mod = compile(cu, "fn x() => 15. != 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 15. != 15.;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);
  }

  SECTION("LT") {
    auto mod = compile(cu, "fn x() => 1. < 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 2. < 2.;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);

    mod = compile(cu, "fn x() => 3. < 2.;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);
  }

  SECTION("LE") {
    auto mod = compile(cu, "fn x() => 1. <= 2.;\n");
    auto fd = cast<FunctionDefn>(mod->members().front());
    EvalResult result;
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 2. <= 2.;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == true);

    mod = compile(cu, "fn x() => 3. <= 2.;\n");
    fd = cast<FunctionDefn>(mod->members().front());
    evalConstExpr(fd->body(), result);
    REQUIRE(result.type == EvalResult::BOOL);
    REQUIRE_FALSE(result.error);
    REQUIRE(result.boolResult == false);
  }
}
