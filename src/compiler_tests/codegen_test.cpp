#include "catch.hpp"
#include "tempest/gen/codegen.hpp"
#include "tempest/gen/cgmodule.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/graph/typestore.hpp"
#include "tempest/opt/basicopts.hpp"

#include "llvm/IR/Verifier.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"

using namespace tempest::sema::graph;
using namespace tempest::gen;
using namespace tempest::opt;
using tempest::source::Location;
using llvm::verifyModule;

TEST_CASE("CodeGen", "[gen]") {
  TypeStore ts;
  llvm::LLVMContext context;
  tempest::source::StringSource source("source.te", "");
  Location loc(&source, 1, 1, 1, 10);

  llvm::InitializeNativeTarget();

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
    CodeGen gen(context);
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
    CodeGen gen(context);
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
    CodeGen gen(context);
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
}
