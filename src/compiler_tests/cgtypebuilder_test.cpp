#include "catch.hpp"
#include "tempest/gen/cgtypebuilder.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/graph/typestore.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/Allocator.h>

using namespace tempest::sema::graph;
using namespace tempest::gen;

TEST_CASE("CGTypeBuilder", "[gen]") {
  llvm::LLVMContext context;
  CGTypeBuilder types(context);

  SECTION("Void") {
    llvm::Type* t = types.get(&VoidType::VOID, {});
    REQUIRE(t->isVoidTy());
    llvm::Type* t2 = types.get(&VoidType::VOID, {});
    REQUIRE(t == t2);
  }

  SECTION("Boolean") {
    llvm::Type* t = types.get(&BooleanType::BOOL, {});
    REQUIRE(t->isIntegerTy());
    llvm::Type* t2 = types.get(&BooleanType::BOOL, {});
    REQUIRE(t == t2);
  }

  SECTION("Integer") {
    llvm::Type* t = types.get(&IntegerType::I16, {});
    REQUIRE(t->isIntegerTy());
    llvm::Type* t2 = types.get(&IntegerType::I16, {});
    REQUIRE(t == t2);
  }

  SECTION("Float") {
    llvm::Type* t = types.get(&FloatType::F32, {});
    REQUIRE(t->isFloatTy());
    llvm::Type* t2 = types.get(&FloatType::F32, {});
    REQUIRE(t == t2);
  }

  SECTION("Tuple") {
    TypeStore store;
    Type* tupleType = store.createTupleType({ &IntegerType::I16, &IntegerType::I32 });
    llvm::Type* t = types.get(tupleType, {});
    REQUIRE(t->isStructTy());
    // REQUIRE(t->getStructNumElements() == 2);
  }
}
