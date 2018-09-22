#ifndef TEMPEST_SUPPORT_HASHING_HPP
#define TEMPEST_SUPPORT_HASHING_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#include <llvm/Support/Allocator.h>

namespace tempest::support {
  inline void hash_combine(size_t& lhs, size_t rhs) {
    lhs ^= rhs + 0x9e3779b9 + (lhs<<6) + (lhs>>2);
  }
}

#endif
