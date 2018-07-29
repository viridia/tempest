#ifndef TEMPEST_AST_LITERAL_HPP
#define TEMPEST_AST_LITERAL_HPP 1

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

namespace tempest::ast {
  using llvm::StringRef;

  /** Character, String, Integer or Float literal. */
  class Literal : public Node {
  public:
    const StringRef value;
    const StringRef suffix;

    Literal(Kind kind, const Location& location, StringRef value, StringRef suffix = "")
      : Node(kind, location)
      , value(value)
      , suffix(suffix)
    {}
  };
}

#endif
