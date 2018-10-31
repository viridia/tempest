#include "catch.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/gen/codegen.hpp"
#include "tempest/gen/cgmodule.hpp"
#include "tempest/gen/cgtarget.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/graph/typestore.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/dataflow.hpp"
#include "tempest/sema/pass/expandspecialization.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "tempest/opt/basicopts.hpp"

#include "llvm/IR/Verifier.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"

using namespace tempest::error;
using namespace tempest::sema::graph;
using namespace tempest::sema::pass;
using namespace tempest::gen;
using namespace tempest::opt;
using tempest::parse::Parser;
using tempest::source::Location;
using llvm::verifyModule;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

namespace {
  /** Parse a module definition and apply buildgraph & nameresolution pass. */
  CGModule* compile(CompilationUnit &cu, CodeGen& gen, const char* srcText) {
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
    ExpandSpecializationPass esPass(cu);
    esPass.process(mod.get());
    esPass.run();
    CompilationUnit::theCU = nullptr;
    REQUIRE(diag.errorCount() == prevErrorCount);

    auto cgMod = gen.createModule("TestMod");
    cgMod->diInit("testmod.te", "");
    gen.genSymbols(cu.symbols());
    cgMod->diFinalize();

    return cgMod;
  }
}

TEST_CASE("CodeGen", "[gen]") {
  TypeStore ts;
  llvm::LLVMContext context;
  tempest::source::StringSource source("source.te", "");
  Location loc(&source, 1, 1, 1, 10);

  llvm::InitializeNativeTarget();

  CGTarget target;
  target.select();

  auto makeIntegerLiteral = [&ts](int64_t val) -> IntegerLiteral* {
    llvm::APInt intVal(32, val, true);
    return new (ts.alloc()) IntegerLiteral(ts.alloc().copyOf(intVal), &IntegerType::I32);
  };

  SECTION("Empty function") {
    ReturnStmt ret(nullptr);
    BlockStmt blk(Location(), { &ret });
    FunctionDefn fdef(Location(), "test");
    fdef.setBody(&blk);
    fdef.setType(ts.createFunctionType(&VoidType::VOID, { &FloatType::F32 }));
    CodeGen gen(context, target);
    auto mod = gen.createModule("TestMod");
    auto fn = gen.genFunction(&fdef, {});
    REQUIRE(fn != nullptr);
    // mod->irModule()->print(llvm::errs(), nullptr);
    REQUIRE_FALSE(verifyModule(*mod->irModule(), &llvm::errs()));
  }

  SECTION("Function with return") {
    ReturnStmt ret(makeIntegerLiteral(1));
    BlockStmt block(loc, {}, &ret);
    FunctionDefn fdef(loc, "testReturn");
    fdef.setBody(&block);
    fdef.setType(ts.createFunctionType(&IntegerType::I32, {}));
    CodeGen gen(context, target);
    auto mod = gen.createModule("TestMod");
    mod->diInit("testmod.te", "");
    auto fn = gen.genFunction(&fdef, {});
    mod->makeEntryPoint(&fdef);
    REQUIRE(fn != nullptr);
    mod->diFinalize();

    REQUIRE_FALSE(verifyModule(*mod->irModule(), &(llvm::errs())));

    BasicOpts opts(mod);
    opts.run(fn);

    // std::error_code EC;
    // auto out = llvm::raw_fd_ostream("out.ll", EC, llvm::sys::fs::F_None);
    // mod->irModule()->print(llvm::errs(), nullptr);
    // mod->irModule()->print(out, nullptr);
  }

  SECTION("Function with expression") {
    BinaryOp add(Expr::Kind::ADD, makeIntegerLiteral(1), makeIntegerLiteral(2), &IntegerType::I32);
    ReturnStmt ret(&add);
    BlockStmt block(loc, {}, &ret);
    FunctionDefn fdef(loc, "testReturn");
    fdef.setBody(&block);
    fdef.setType(ts.createFunctionType(&IntegerType::I32, {}));
    CodeGen gen(context, target);
    auto mod = gen.createModule("TestMod");
    mod->diInit("testmod.te", "");
    auto fn = gen.genFunction(&fdef, {});
    mod->makeEntryPoint(&fdef);
    REQUIRE(fn != nullptr);
    mod->diFinalize();

    REQUIRE_FALSE(verifyModule(*mod->irModule(), &(llvm::errs())));

    BasicOpts opts(mod);
    opts.run(fn);

    // Note that the constants will be folded in the output.
    // std::error_code EC;
    // auto out = llvm::raw_fd_ostream("out.ll", EC, llvm::sys::fs::F_None);
    // mod->irModule()->print(llvm::errs(), nullptr);
    // mod->irModule()->print(out, nullptr);
  }

  SECTION("compile constructor call") {
    CompilationUnit cu;
    CodeGen gen(context, target);
    auto cgMod = compile(cu, gen,
      "class A {\n"
      "  new() {}\n"
      "}\n"
      "fn test {\n"
      "  return A();\n"
      "}\n"
    );

    // cgMod->irModule()->print(llvm::errs(), nullptr);
    REQUIRE_FALSE(verifyModule(*cgMod->irModule(), &(llvm::errs())));

    auto clsDecA = cgMod->irModule()->getGlobalVariable("test.mod.A::cldesc");
    REQUIRE(clsDecA != nullptr);

    auto ctorA = cgMod->irModule()->getFunction("test.mod.A.new");
    REQUIRE(ctorA != nullptr);
    REQUIRE(ctorA->size() > 0);

    auto testFn = cgMod->irModule()->getFunction("test.mod.test->test.mod.A");
    REQUIRE(testFn != nullptr);
    REQUIRE(testFn->size() > 0);
  }

  SECTION("field initialization") {
    CompilationUnit cu;
    CodeGen gen(context, target);
    auto cgMod = compile(cu, gen,
      "class A {\n"
      "  size: int = 0;\n"
      "}\n"
    );

    // cgMod->irModule()->print(llvm::errs(), nullptr);
    REQUIRE_FALSE(verifyModule(*cgMod->irModule(), &(llvm::errs())));
  }

  SECTION("const field initialization") {
    CompilationUnit cu;
    CodeGen gen(context, target);
    auto cgMod = compile(cu, gen,
      "class A {\n"
      "  const size: int = 0;\n"
      "}\n"
    );

    // cgMod->irModule()->print(llvm::errs(), nullptr);
    REQUIRE_FALSE(verifyModule(*cgMod->irModule(), &(llvm::errs())));
  }

  SECTION("method call") {
    CompilationUnit cu;
    CodeGen gen(context, target);
    auto cgMod = compile(cu, gen,
      "class A {\n"
      "  final a() => 0;\n"
      "}\n"
      "class B {\n"
      "  b() {\n"
      "    let x = A();"
      "    let y = x.a();"
      "    return y;"
      "  }\n"
      "}\n"
    );

    // cgMod->irModule()->print(llvm::errs(), nullptr);
    REQUIRE_FALSE(verifyModule(*cgMod->irModule(), &(llvm::errs())));
  }

  SECTION("flexalloc base") {
    CompilationUnit cu;
    CodeGen gen(context, target);
    auto cgMod = compile(cu, gen,
      "final class A extends FlexAlloc[u8] {\n"
      "  size: i32 = 0;\n"
      "  new() { self = __alloc(5); size = 5; }\n"
      "  get empty: bool => size == 0;\n"
      "}\n"
      "fn b() {"
      "  A()"
      "}"
    );

    // cgMod->irModule()->print(llvm::errs(), nullptr);
    REQUIRE_FALSE(verifyModule(*cgMod->irModule(), &(llvm::errs())));
  }

  SECTION("union member field") {
    CompilationUnit cu;
    CodeGen gen(context, target);
    auto cgMod = compile(cu, gen,
      "class A {\n"
      "  x: i32 | void;\n"
      "  a!() {\n"
      "    x = 0;\n"
      "  }\n"
      "}\n"
    );

    BasicOpts opts(cgMod);
    for (auto& fn : cgMod->irModule()->functions()) {
      opts.run(&fn);
    }

    // cgMod->irModule()->print(llvm::errs(), nullptr);
    REQUIRE_FALSE(verifyModule(*cgMod->irModule(), &(llvm::errs())));
  }
}
