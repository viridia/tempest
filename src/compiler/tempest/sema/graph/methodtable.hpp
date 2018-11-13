#ifndef TEMPEST_SEMA_GRAPH_METHODTABLE_HPP
#define TEMPEST_SEMA_GRAPH_METHODTABLE_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

namespace tempest::sema::graph {
  class Type;
  class FunctionDefn;

  struct MethodTableEntry {
    FunctionDefn* method;
    ArrayRef<const Type*> typeArgs;
  };

  typedef std::vector<MethodTableEntry> MethodTable;
}

#endif
