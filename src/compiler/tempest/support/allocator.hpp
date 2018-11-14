#ifndef TEMPEST_SUPPORT_ALLOCATOR_HPP
#define TEMPEST_SUPPORT_ALLOCATOR_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#include <llvm/ADT/APInt.h>
#include <llvm/Support/Allocator.h>

namespace tempest::support {
  class BumpPtrAllocator : public llvm::MallocAllocator {
  public:
    /** Make a copy of this array within the allocator. */
    template<class T> llvm::ArrayRef<T> copyOf(llvm::ArrayRef<T> array) {
      if (array.size() == 0) {
        return array;
      }
      auto data = static_cast<T*>(Allocate(array.size() * sizeof (T), sizeof (T)));
      std::copy(array.begin(), array.end(), data);
      return llvm::ArrayRef((T*) data, array.size());
    }

    /** Make a copy of this array within the allocator. */
    template<class T> llvm::ArrayRef<T> copyOf(const llvm::SmallVectorImpl<T>& array) {
      if (array.size() == 0) {
        return array;
      }
      auto data = static_cast<T*>(Allocate(array.size() * sizeof (T), sizeof (T)));
      std::copy(array.begin(), array.end(), data);
      return llvm::ArrayRef((T*) data, array.size());
    }

    /** Make a copy of this string within the allocator. */
    llvm::StringRef copyOf(llvm::StringRef str) {
      if (str.size() == 0) {
        return str;
      }
      auto data = static_cast<char*>(Allocate(str.size(), 1));
      std::copy(str.begin(), str.end(), data);
      return llvm::StringRef((char*) data, str.size());
    }

    /** Make a copy of this APInt within the allocator. */
    llvm::ArrayRef<llvm::APInt::WordType> copyOf(const llvm::APInt &intVal) {
      auto data = static_cast<llvm::APInt::WordType*>(
          Allocate(intVal.getNumWords() * sizeof(llvm::APInt::WordType),
              sizeof(llvm::APInt::WordType)));
      std::copy(intVal.getRawData(), intVal.getRawData() + intVal.getNumWords(), data);
      return llvm::ArrayRef((llvm::APInt::WordType*) data, intVal.getNumWords());
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
