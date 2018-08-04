// ============================================================================
// sema/graph/member.h: Member defintion.
// ============================================================================

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
#define TEMPEST_SEMA_GRAPH_MEMBER_HPP 1

#ifndef TEMPEST_CONFIG_HPP
  #include "config.h"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#include <memory>
#include <unordered_map>

namespace tempest::ast {
  class Node;
}

namespace tempest::sema::graph {
  /** Base class for all members within a scope. */
  class Member {
  public:
    enum class Kind {
      // Definitions
      TYPE = 1,
      TYPE_PARAM,
      CONST_DEF,
      LET_DEF,
      ENUM_VAL,
      FUNCTION,
      FUNCTION_PARAM,

      MODULE,

      // Internal types
      SPECIALIZED,   // A combination of generic member and type arguments for it
      TUPLE_MEMBER,

      COUNT,
    };

    const Kind kind;

    Member(Kind kind, const llvm::StringRef& name)
      : kind(kind)
      , _name(name.begin(), name.end())
      , _ast(nullptr)
    {}

    virtual ~Member() {}

    /** Abstract syntax tree for this member. */
    const ast::Node* ast() const { return _ast; }
    void setAst(const ast::Node* ast) { _ast = ast; }

    /** The name of this member. */
    llvm::StringRef name() const { return _name; }

    /** Scope in which this module was defined. */
    virtual Member* definedIn() const { return nullptr; }

    /** Dynamic casting support. */
    static bool classof(const Member* m) { return true; }

  protected:
    const std::string _name;
    const ast::Node* _ast;
  };

  /** List of members. */
  typedef std::vector<Member*> MemberList;
  typedef llvm::ArrayRef<Member*> MemberArray;

  /** Function to pretty-print a Defn graph. */
  void format(::std::ostream& out, const Member* m, bool pretty);
}

#endif
