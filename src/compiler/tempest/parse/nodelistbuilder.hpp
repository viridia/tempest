#ifndef TEMPEST_PARSE_NODELISTBUILDER_HPP
#define TEMPEST_PARSE_NODELISTBUILDER_HPP 1

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_H
  #include "tempest/source/location.h"
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#ifndef LLVM_ADT_SMALLVECTOR_H
  #include <llvm/ADT/SmallVector.h>
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

namespace tempest::parse {
  using tempest::ast::Node;
  using tempest::source::Location;
  using llvm::ArrayRef;

  /** Helper for building node lists in an alloc. */
  class NodeListBuilder {
  public:
    NodeListBuilder(llvm::BumpPtrAllocator& alloc) : _alloc(alloc) {}
    NodeListBuilder& append(Node* n) {
      _nodes.push_back(n);
      return *this;
    }

    size_t size() const {
      return _nodes.size();
    }

    /** Return a location which spans all of the nodes. */
    Location location() const {
      Location loc;
      for (Node* n : _nodes) {
        loc |= n->location;
      }
      return loc;
    }

    Node* operator[](int index) const {
      return _nodes[index];
    }

    ArrayRef<const Node*> build() const {
      if (_nodes.empty()) {
        return ArrayRef<const Node*>();
      }
      Node** data = reinterpret_cast<Node**>(
        _alloc.Allocate(sizeof(Node*) * _nodes.size(), sizeof (Node*)));
      ArrayRef<const Node*> result(data, _nodes.size());
      std::copy(_nodes.begin(), _nodes.end(), data);
      return result;
    }
  private:
    llvm::BumpPtrAllocator& _alloc;
    llvm::SmallVector<Node*, 8> _nodes;
  };
}

#endif
