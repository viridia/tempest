#include "catch.hpp"
#include "tempest/gen/cgmodule.hpp"
#include "tempest/sema/graph/module.hpp"

#include <llvm/Support/TargetSelect.h>
#include <memory>

using namespace tempest::sema::graph;
using namespace tempest::gen;

TEST_CASE("CGModule", "[gen]") {
  tempest::source::StringSource source("source.te", "");
  // Module m(&source, "TestModule");
  llvm::LLVMContext context;

  llvm::InitializeNativeTarget();

  SECTION("new") {
    CGModule cgMod("TestModule", context);
    REQUIRE(cgMod.irModule() != NULL);
    REQUIRE(cgMod.irModule()->getModuleIdentifier() == "TestModule");
    // REQUIRE(cgMod.irModule()->getSourceFileName() == "source.te");
  }
}
