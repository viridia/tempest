#ifndef TEMPEST_SEMA_NAMES_CLOSESTNAME_HPP
#define TEMPEST_SEMA_NAMES_CLOSESTNAME_HPP 1

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

namespace tempest::sema::names {
  /** Compute the closest matching name to the one found. */
  struct ClosestName {
    llvm::StringRef targetName;
    llvm::StringRef bestMatch;
    unsigned bestDistance;

    ClosestName(llvm::StringRef targetName)
      : targetName(targetName)
      , bestDistance(std::min(targetName.size(), size_t(10)))
    {}
    ClosestName() = delete;
    ClosestName(const ClosestName&) = delete;

    void operator()(llvm::StringRef name) {
      auto distance = targetName.edit_distance(name, true, 10);
      if (distance < bestDistance) {
        bestMatch = name;
        bestDistance = distance;
      }
    }
  };
}

#endif
