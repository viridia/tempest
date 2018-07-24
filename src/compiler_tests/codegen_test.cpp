#include "catch.hpp"
#include "tempest/gen/codegen.h"
#include "tempest/gen/cgmodule.h"
#include "tempest/sema/graph/defn.h"
#include "tempest/sema/graph/expr_stmt.h"
#include "tempest/sema/graph/expr_literal.h"
#include "tempest/sema/graph/primitivetype.h"
#include "tempest/sema/graph/typestore.h"
#include "tempest/opt/basicopts.h"

#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

using namespace tempest::sema::graph;
using namespace tempest::gen;
using namespace tempest::opt;
using tempest::source::Location;
using llvm::verifyModule;

TEST_CASE("CodeGen", "[gen]") {
  TypeStore ts;
  llvm::LLVMContext context;

  SECTION("Empty function") {
    ReturnStmt ret(nullptr);
    BlockStmt blk(Location(), { &ret });
    FunctionDefn fdef(Location(), "test");
    fdef.setBody(&blk);
    fdef.setType(ts.createFunctionType(&VoidType::VOID, { &FloatType::F32 }));
    CodeGen gen(context);
    auto mod = gen.createModule("TestMod");
    auto fn = gen.genFunction(&fdef);
    REQUIRE(fn != nullptr);

    verifyModule(*mod->irModule(), &(llvm::errs()));
  }

  SECTION("Function with return") {
    IntegerLiteral intLit(1, &IntegerType::I32);
    ReturnStmt ret(&intLit);
    BlockStmt block(Location(), { &ret });
    FunctionDefn fdef(Location(), "testReturn");
    fdef.setBody(&block);
    fdef.setType(ts.createFunctionType(&IntegerType::I32, {}));
    CodeGen gen(context);
    auto mod = gen.createModule("TestMod");
    auto fn = gen.genFunction(&fdef);
    mod->makeEntryPoint(&fdef);
    REQUIRE(fn != nullptr);

    verifyModule(*mod->irModule(), &(llvm::errs()));

    BasicOpts opts(mod);
    opts.run(fn);

    std::error_code EC;
    auto out = llvm::raw_fd_ostream("out.ll", EC, llvm::sys::fs::F_None);
    mod->irModule()->print(llvm::errs(), nullptr);
    mod->irModule()->print(out, nullptr);
  }
}
