#ifndef TEMPEST_COMMON_HPP
#define TEMPEST_COMMON_HPP 1

// External headers that are used in many places

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/Casting.h>
#include <ostream>
#include <algorithm>

using llvm::dyn_cast;
using llvm::dyn_cast_or_null;
using llvm::cast;
using llvm::cast_or_null;
using llvm::ArrayRef;
using llvm::SmallVector;
using llvm::StringRef;

#endif
