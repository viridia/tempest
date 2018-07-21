#ifndef TEMPEST_GEN_CGGLINKAGENAME_H
#define TEMPEST_GEN_CGGLINKAGENAME_H 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_H
  #include "tempest/sema/graph/defn.h"
#endif

namespace tempest::gen {
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;

  void getDefnLinkageName(std::string& out, const Member* m, llvm::ArrayRef<Type*> typeArgs);
  void getTypeLinkageName(std::string& out, const Type* ty, llvm::ArrayRef<Type*> typeArgs);

  inline void getLinkageName(std::string& out, const Member* m) {
    getDefnLinkageName(out, m, {});
  }
  inline void getLinkageName(std::string& out, const Type* ty) {
    getTypeLinkageName(out, ty, {});
  }
}

#endif
