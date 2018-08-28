#ifndef TEMPEST_AST_MODULE_HPP
#define TEMPEST_AST_MODULE_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

namespace tempest::ast {
  using llvm::StringRef;

  /** AST node for a module. */
  class Module : public Node {
  public:
    NodeList members;
    NodeList imports;

    Module(const Location& location)
      : Node(Kind::MODULE, location)
    {}
  };

  /** Node type representing an import or export. */
  class Import : public Node {
  public:
    NodeList members; // List of imported members.
    llvm::StringRef path; // Module path expression.
    int32_t relative; // If non-zero, indicates how many leading dots were on the path.

    Import(const Location& location, llvm::StringRef path, int32_t relative)
      : Node(Kind::IMPORT, location)
      , path(path)
      , relative(relative)
    {}

    // For export statements
    Import(Kind kind, const Location& location, llvm::StringRef path, int32_t relative)
      : Node(kind, location)
      , path(path)
      , relative(relative)
    {}
  };
}

#endif
