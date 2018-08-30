#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
#define TEMPEST_SEMA_GRAPH_EXPR_HPP 1

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
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
      // BUILTIN_ATTR,

      // Literals
      BOOLEAN_LITERAL,
      INTEGER_LITERAL,
      FLOAT_LITERAL,
      DOUBLE_LITERAL,
      STRING_LITERAL,
      ARRAY_LITERAL,

      // Unary Operators
      NOT,
      NEGATE,
      COMPLEMENT,

      // Binary Operators
      ADD,
      SUBTRACT,
      MULTIPLY,
      DIVIDE,
      REMAINDER,
      LSHIFT,
      RSHIFT,
      BIT_AND, BIT_OR, BIT_XOR,

      // Relational operators
      EQ,
      NE,
      LT,
      LE,
      GT,
      GE,
      REF_NE,
      REF_EQ,

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
      LOGICAL_AND, LOGICAL_OR,
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
      CALL,                         // Function call

    // Member references
      VAR_REF,                      // Reference to a variable
      TYPE_REF,                     // Reference to a type name
      TYPE_REF_OVERLOAD,            // Reference to an overloaded type name
      FUNCTION_REF,                 // Reference to a function
      FUNCTION_REF_OVERLOAD,        // Reference to an overloaded function name
      // MEMBER_LIST,                  // List of members - results of a lookup
      // DEFN_REF,                     // Reference to a definition (produced from MEMBER_LIST).
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
      LOCAL_VAR,
    //   IMPORT,
      SWITCH, SWITCH_CASE,
      MATCH, MATCH_PATTERN,
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

      INFIX_START = ADD,
      INFIX_END = RSHIFT,
      RELATIONAL_START = EQ,
      RELATIONAL_END = GE,
    };

    const Kind kind;
    const Location location;
    Type* type = nullptr;

    Expr(Kind kind, Location location) : kind(kind), location(location) {}
    Expr() = delete;
    Expr(const Expr& src) = delete;

    /** Implemente Locatable. */
    const Location& getLocation() const { return location; }

    // Return the name of the specified kind.
    static const char* KindName(Kind kind);

    /** Return true if this type is an error sentinel. */
    static bool isError(const Expr* e) {
      return e == nullptr || e->kind == Kind::INVALID;
    }

    /** Dynamic casting support. */
    static bool classof(const Expr* t) { return true; }

    static Expr ERROR;
  };

  /** Function to print a type. */
  void format(::std::ostream& out, const Expr* e);

  // Built-in values

  /** The 'super' keyword. */
  class SuperExpr : public Expr {
  public:
    SuperExpr(Location location): Expr(Kind::SUPER, location) {}

    /** Dynamic casting support. */
    static bool classof(const SuperExpr* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SUPER; }
  };

  /** The 'self' keyword. */
  class SelfExpr : public Expr {
  public:
    SelfExpr(Location location): Expr(Kind::SELF, location) {}

    /** Dynamic casting support. */
    static bool classof(const SelfExpr* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::SELF; }
  };

  // Identifiers

  /** Reference to a definition, with optional stem expression. */
  class DefnRef : public Expr {
  public:
    /** Member reference - function, variable, etc. */
    Member* defn = nullptr;

    /** Stem expression */
    Expr* stem = nullptr;

    DefnRef(Expr::Kind kind, Location location): Expr(kind, location) {}
    DefnRef(Expr::Kind kind, Location location, Member* defn, Expr* stem = nullptr)
      : Expr(kind, location)
      , defn(defn)
      , stem(stem)
    {}

    /** Dynamic casting support. */
    static bool classof(const DefnRef* e) { return true; }
    static bool classof(const Expr* e) {
      return e->kind == Kind::VAR_REF
          || e->kind == Kind::FUNCTION_REF
          || e->kind == Kind::TYPE_REF;
    }
  };

  /** Possibly ambiguous result of a name lookup, waiting for inference to resolve. */
  class MemberListExpr : public Expr {
  public:
    /** The name of the members that were searched for. */
    llvm::StringRef name;

    /** List of members. */
    const llvm::ArrayRef<Member*> members;

    /** Stem expression */
    Expr* stem = nullptr;

    MemberListExpr(
        Expr::Kind kind,
        Location location,
        llvm::StringRef name,
        const llvm::ArrayRef<Member*>& members)
      : Expr(kind, location)
      , name(name)
      , members(members)
    {}

    /** Dynamic casting support. */
    static bool classof(const DefnRef* e) { return true; }
    static bool classof(const Expr* e) {
      return e->kind == Kind::TYPE_REF_OVERLOAD || e->kind == Kind::FUNCTION_REF_OVERLOAD;
    }
  };
}

#endif
