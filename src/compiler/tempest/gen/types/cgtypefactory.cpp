#include "tempest/gen/types/cgtypefactory.h"
#include "tempest/sema/graph/primitivetype.h"
#include <llvm/IR/DerivedTypes.h>
#include <assert.h>

namespace tempest::gen::types {
  using namespace tempest::sema::graph;

  CGType* CGTypeFactory::get(const Type* ty) {
    const TypeMap::const_iterator it = _types.find(ty);
    if (it != _types.end()) {
      return it->second;
    }

    CGType* result;
    switch (ty->kind) {
      case Type::Kind::INTEGER: {
        auto intTy = static_cast<const IntegerType*>(ty);
        result = new (_alloc) CGType(ty, llvm::IntegerType::get(_context, intTy->bits()));
        break;
      }

      default:
        assert(false && "Invalid type argument");
        return NULL;
    }

    _types[ty] = result;
    return result;
  }
}
