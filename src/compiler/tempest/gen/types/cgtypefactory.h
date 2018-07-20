#ifndef TEMPEST_GEN_TYPES_CGTYPEFACTORY_H
#define TEMPEST_GEN_TYPES_CGTYPEFACTORY_H 1

#ifndef TEMPEST_GEN_TYPES_CGTYPE_H
  #include "tempest/gen/types/cgtype.h"
#endif

#include <unordered_map>

namespace tempest::gen::types {
  using tempest::sema::graph::Type;

  /** Maps Tempest type expressions to LLVM types. */
  class CGTypeFactory {
  public:
    CGTypeFactory(llvm::LLVMContext& context, llvm::BumpPtrAllocator &alloc)
      : _context(context)
      , _alloc(alloc)
    {}

    CGType* get(const Type*);

  private:
    typedef std::unordered_map<const Type*, CGType*> TypeMap;

    llvm::LLVMContext& _context;
    llvm::BumpPtrAllocator& _alloc;
    TypeMap _types;
  };
}

#endif
