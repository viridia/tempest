#ifndef TEMPEST_SEMA_GRAPH_EXPR_H
#define TEMPEST_SEMA_GRAPH_EXPR_H 1

#ifndef TEMPEST_SOURCE_LOCATION_H
  #include "tempest/source/location.h"
#endif

#ifndef LLVM_ADT_APINT_H
  #include <llvm/ADT/APInt.h>
#endif

#ifndef LLVM_ADT_APFLOAT_H
  #include <llvm/ADT/APFloat.h>
#endif

namespace tempest::sema::graph {
  using tempest::source::Location;
  using tempest::source::Locatable;

  class Member;
  class Type;

  class Expr : public Locatable {
  public:
    enum class Kind {
      INVALID = 0,

      VOID,
      ID,
      SELF,
      SUPER,
      NS_LITERAL,       // A literal reference to a module or package
      // BUILTIN_ATTR,

      // Literals
      TRUE_LITERAL,
      FALSE_LITERAL,
      INTEGER_LITERAL,
      FLOAT_LITERAL,
      DOUBLE_LITERAL,
      STRING_LITERAL,
      ARRAY_LITERAL,

      // Unary Operators

      // Assignment operators
      ASSIGN,

      // Cast operators
    //   UP_CAST,                      # Cast from subclass to base type.
    //   DOWN_CAST,                    # Cast from base type to subclass (unconditional)
    //   TRY_CAST,                     # Cast from base to subclass, throw if fail.
    //   IFACE_CAST,                   # Cast from type to interface which it is known to support.
    //   DYN_IFACE_CAST,               # Cast from type to interface, throw if fail.
    //   UNION_CTOR_CAST,              # Cast to a union type
    //   UNION_MEMBER_CAST,            # Cast from a union type
    //   BOX_CAST,                     # Cast from value type to reference type.
    //   UNBOX_CAST,                   # Cast from reference type to value type.
    // #  DYNAMIC_CAST,                 # Cast from base/iface to subclass, null if fail
    // #  EXPR_TYPE(QualCast)       // Cast that changes only qualifiers (no effect)
    // #  EXPR_TYPE(UnionMemberCast)// Cast from a union type.
    // #  EXPR_TYPE(CheckedUnionMemberCast)// Cast from a union type, with type check.
    //   REP_CAST,                     # Cast between types that have the exact same machine representation (example: char and u32).
    //   TRUNCATE,                     # Number truncation.
    //   SIGN_EXTEND,                  # Signed integer extend.
    //   ZERO_EXTEND,                  # Unsigned integer extend.
    //   FP_EXTEND,                    # Floating-point extend.
    //   FP_TRUNC,                     # Floating-point truncate.
    //   INT_TO_FLOAT,                 # Convert integer to float.
    //   INTERFACE_DATA,               # Extract interface payload.
    // # EXPR_TYPE(BitCast)        // Reinterpret cast

    // Misc operators
    //   LOGICAL_AND, LOGICAL_OR,
    //   RANGE,
    //   PACK,
    //   AS_TYPE,
    //   IS_TYPE,
    // #  IN,
    // #  NOT_IN,
    //   RETURNS,
    //   PROG,                         # Evaluate a list of expressions and return the last one.
    //   ANON_FN,                      # Anonymous function
    //   TUPLE_MEMBER,                 # Return the Nth member of a tuple.
    //   # TPARAM_DEFAULT,

    //   # Evaluations and Invocations
    //   CALL,                         # Function call156

    // Member references
      MEMBER_LIST,                  // List of members - results of a lookup
      DEFN_REF,                     // Reference to a definition (produced from MEMBER_LIST).
    //   PRIVATE_NAME,                 # Reference to private member (.a)
    //   FLUENT_MEMBER,                # Sticky reference to member (a.{b;c;d})
    //   TEMP_VAR,                     # Reference to an anonymous, temporary variable.
    //   EXPLICIT_SPECIALIZE,          # An explicit specialization that could not be resolved at name-lookup time.

      // Statements
      BLOCK,                        // Block statement
      IF,
      WHILE,
      LOOP,
      // FOR,                          // C-style for
      FOR_IN,                       // for x in y
      THROW,
    //   TRY,
      RETURN,
    //   # YIELD,
      BREAK,
      CONTINUE,
    //   LOCAL_DECL,
    //   IMPORT,
      SWITCH, SWITCH_CASE,
      MATCH, MATCH_PATTERN,
    //   INTRINSIC,                    # Call to built-in function
      UNREACHABLE,                  // Indicates control cannot reach this point

    //   # Other syntax
    //   KEYWORD_ARG,

    //   # Constants
    //   CONST_OBJ,                    # A static, immutable object
    //   CONST_ARRAY,                  # A static, immutable array

    //   # Type expressions
    //   MODIFIED,

    //   # Lowered expressions
    //   CALL_STATIC,                  # Call directly
    //   CALL_INDIRECT,                # Call an instance method by table index
    //   CALL_REQUIRED,                # Call a required method by table index
    //   CALL_CTOR,                    # Construct object and call constructor
    //   CALL_EXPR,                    # Call where the callable is an expression of function type
    //   CALL_INTRINSIC,               # Generate code from an intrinsic
    //   FNREF_STATIC,                 # Reference to a static function
    //   FNREF_INDIRECT,               # Reference to a function looked up by method index
    //   FNREF_REQUIRED,               # Reference to a required method looked up by method index
    //   GC_ALLOC,                     # Allocate an object on the heap
    //   TYPEDESC,                     # Direct reference to a type descriptor
    //   PTR_DEREF,                    # Pointer dereference.
    //   ADDRESS_OF,                   # Address of a value, guaranteed not to be on the heap.

    //   # Non-leaf types
    //   BASE,
    //   OPER,
    //   UNARY_OP,
    //   BINARY_OP,
    //   CAST_OP,
    };

    const Kind kind;
    const Location location;

    Expr(Kind kind, Location location) : kind(kind), location(location) {}
    Expr() = delete;
    Expr(const Expr& src) = delete;

    /** Implemente Locatable. */
    const Location& getLocation() const { return location; }

    /** Return true if this type is an error sentinel. */
    static bool isError(const Expr* e) {
      return e == nullptr || e->kind == Kind::INVALID;
    }

    /** Dynamic casting support. */
    static bool classof(const Expr* t) { return true; }

    static Expr ERROR;
  };

  // Built-in values

  /** The 'super' keyword. */
  class SuperExpr : public Expr {
  public:
    SuperExpr(Location location): Expr(Kind::SUPER, location) {}

    /** Actual type of 'super' */
    Type* type() const { return _type; }
    void setType(Type* type) { _type = type; }

    /** Dynamic casting support. */
    static bool classof(const SuperExpr* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SUPER; }

  private:
    Type* _type = nullptr;
  };

  /** The 'self' keyword. */
  class SelfExpr : public Expr {
  public:
    SelfExpr(Location location): Expr(Kind::SELF, location) {}

    /** Actual type of 'super' */
    Type* type() const { return _type; }
    void setType(Type* type) { _type = type; }

    /** Dynamic casting support. */
    static bool classof(const SelfExpr* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SELF; }

  private:
    Type* _type = nullptr;
  };

  // Identifiers

  /** Reference to a definition, with optional stem expression. */
  class DefnRef : public Expr {
  public:
    DefnRef(Location location): Expr(Kind::DEFN_REF, location) {}

    /** Member reference - function, variable, etc. */
    Member* defn() const { return _defn; }
    void setMember(Member* m) { _defn = m; }

    /** Stem expression */
    Expr* stem() const { return _stem; }
    void setStem(Expr* stem) { _stem = stem; }

    /** Cached type of the stem */
    Type* stemType() const { return _stemType; }
    void setStemType(Type* type) { _stemType = type; }

    /** Dynamic casting support. */
    static bool classof(const DefnRef* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::DEFN_REF; }

  private:
    Member* _defn = nullptr;
    Expr* _stem = nullptr;
    Type* _stemType = nullptr;
  };

  /** Possibly ambiguous result of a name lookup, waiting for inference to resolve. */
  class MemberListExpr : public Expr {
  public:
    enum class ListType {
      INCOMPLETE,     // We haven't actually filled in the members yet.
      NAMESPACE,      // Package or module
      FUNCTION,       // List of functions, possibly overloaded
      TYPE,           // List of types, possibly overloaded
      VARIABLE,       // A reference to a variable, parameter, or property
    };

    MemberListExpr(Location location, llvm::StringRef name)
      : Expr(Kind::MEMBER_LIST, location)
      , _name(name)
      , _listType(ListType::INCOMPLETE)
    {}

    /** The name of the members that were searched for. */
    const llvm::StringRef& name() const { return _name; }

    /** List of members. */
    const llvm::ArrayRef<Member*>& members() const { return _members; }
    void setMembers(llvm::ArrayRef<Member*> members) { _members = members; }

    /** Stem expression */
    Expr* stem() const { return _stem; }
    void setStem(Expr* stem) { _stem = stem; }

    /** Cached type of the stem */
    Type* stemType() const { return _stemType; }
    void setStemType(Type* type) { _stemType = type; }

    /** What kind of members are in this list. */
    ListType listType() const { return _listType; }
    void setListType(ListType listType) { _listType = listType; }

    /** Dynamic casting support. */
    static bool classof(const DefnRef* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::MEMBER_LIST; }

  private:
    llvm::StringRef _name;
    llvm::ArrayRef<Member*> _members;
    ListType _listType;
    Expr* _stem = nullptr;
    Type* _stemType = nullptr;
  };
}

#endif
