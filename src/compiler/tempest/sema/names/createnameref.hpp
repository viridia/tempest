#ifndef TEMPEST_SEMA_NAMES_CREATENAMEREF_HPP
#define TEMPEST_SEMA_NAMES_CREATENAMEREF_HPP 1

#ifndef TEMPEST_GRAPH_SYMBOLTABLE_HPP
  #include "tempest/sema/graph/symboltable.hpp"
#endif

namespace tempest::sema::graph {
  class Defn;
  class Expr;
}

namespace tempest::source {
  struct Location;
}

namespace tempest::support {
  class BumpPtrAllocator;
}

namespace tempest::sema::names {
  using tempest::sema::graph::Defn;
  using tempest::sema::graph::Expr;
  using tempest::sema::graph::MemberLookupResultRef;
  using tempest::source::Location;

  Expr* createNameRef(
      support::BumpPtrAllocator& alloc,
      const Location& loc,
      const MemberLookupResultRef& result,
      Defn* subject,
      Expr* stem,
      bool preferPrivate,
      bool useADL);
}

#endif
