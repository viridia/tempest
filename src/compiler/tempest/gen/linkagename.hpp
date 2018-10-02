#ifndef TEMPEST_GEN_CGGLINKAGENAME_HPP
#define TEMPEST_GEN_CGGLINKAGENAME_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

namespace tempest::gen {
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;

  void getDefnLinkageName(std::string& out, const Member* m, llvm::ArrayRef<const Type*> typeArgs);
  void getTypeLinkageName(std::string& out, const Type* ty, llvm::ArrayRef<const Type*> typeArgs);

  inline void getLinkageName(
      std::string& out,
      const Member* m,
      llvm::ArrayRef<const Type*> typeArgs) {
    getDefnLinkageName(out, m, typeArgs);
  }
  inline void getLinkageName(
      std::string& out,
      const Type* ty,
      llvm::ArrayRef<const Type*> typeArgs) {
    getTypeLinkageName(out, ty, typeArgs);
  }
}

#endif
