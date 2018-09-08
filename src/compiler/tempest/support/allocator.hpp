#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
#define TEMPEST_SUPPORT_ALLOCATOR_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#include <llvm/Support/Allocator.h>

namespace tempest::support {
  class BumpPtrAllocator : public llvm::MallocAllocator {
  public:
    /** Make a copy of this array within the allocator. */
    template<class T> llvm::ArrayRef<T> copyOf(llvm::ArrayRef<T> array) {
      auto data = static_cast<T*>(Allocate(array.size() * sizeof (T), sizeof (T)));
      std::copy(array.begin(), array.end(), data);
      return llvm::ArrayRef((T*) data, array.size());
    }

    /** Make a copy of this array within the allocator. */
    template<class T> llvm::ArrayRef<T> copyOf(const llvm::SmallVectorImpl<T>& array) {
      auto data = static_cast<T*>(Allocate(array.size() * sizeof (T), sizeof (T)));
      std::copy(array.begin(), array.end(), data);
      return llvm::ArrayRef((T*) data, array.size());
    }

    /** Make a copy of this string within the allocator. */
    llvm::StringRef copyOf(llvm::StringRef str) {
      auto data = static_cast<char*>(Allocate(str.size(), 1));
      std::copy(str.begin(), str.end(), data);
      return llvm::StringRef((char*) data, str.size());
    }
  };
}

inline void *operator new(size_t Size, tempest::support::BumpPtrAllocator &Allocator) {
  struct S {
    char c;
    union {
      double D;
      long double LD;
      long long L;
      void *P;
    } x;
  };
  return Allocator.Allocate(Size, std::min((size_t)llvm::NextPowerOf2(Size), offsetof(S, x)));
}
inline void operator delete(void *, tempest::support::BumpPtrAllocator &) {}

#endif