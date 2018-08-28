#ifndef TEMPEST_GEN_INSTR_HPP
#define TEMPEST_GEN_INSTR_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

namespace tempest::sema::graph {
  class Member;
  class Type;
}

namespace tempest::gen {
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;
  using tempest::source::Location;
  using tempest::source::Locatable;

  /** Represents a lowered instruction. */
  class Instr : public Locatable {
  public:
    enum class Kind {
      INVALID = 0,

      // Literals
      CONST_TRUE,
      CONST_FALSE,
      CONST_INTEGER,
      CONST_FLOAT,

      // Unary Operators
      I_NEG,
      F_NEG,

      // Infix Binary Operators
      I_ADD,
      F_ADD,
      I_SUB,
      F_SUB,
      S_MUL,
      U_MUL,
      F_MUL,
      S_DIV,
      U_DIV,
      F_DIV,
      S_REM,
      U_REM,
      F_REM,
      // LSHIFT,
      // RSHIFT,

      // Relational operators
      I_EQ,
      F_EQ,
      I_NE,
      F_NE,
      S_LT,
      U_LT,
      F_LT,
      S_LE,
      U_LE,
      F_LE,
      S_GT,
      U_GT,
      F_GT,
      S_GE,
      U_GE,
      F_GE,

      // Assignment operators
      ASSIGN,

      // Cast operators

      // Member references
      LVALUE_REF,                   // Reference to an l-value.

      // Statements
      BLOCK,                        // Block statement
      IF,
      WHILE,
      LOOP,
      // FOR,                          // C-style for
      // FOR_IN,                       // for x in y
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

    //   # Constants
    //   CONST_OBJ,                    # A static, immutable object
    //   CONST_ARRAY,                  # A static, immutable array

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
    };

    const Kind kind;
    const Location location;
    const Type* type;

    Instr(Kind kind, Location location, Type* type)
      : kind(kind)
      , location(location)
      , type(type)
    {}
    Instr() = delete;
    Instr(const Instr& src) = delete;

    /** Implemente Locatable. */
    const Location& getLocation() const { return location; }

    /** Return true if this type is an error sentinel. */
    static bool isError(const Instr* e) {
      return e == nullptr || e->kind == Kind::INVALID;
    }

    /** Dynamic casting support. */
    static bool classof(const Instr* t) { return true; }

    static Instr ERROR;
  };

  // Built-in values

  /** Unary operator. */
  class UnaryOp : public Instr {
  public:
    Instr* arg;

    UnaryOp(Kind kind, Location location, Type* type, Instr* arg)
      : Instr(kind, location, type)
      , arg(arg)
    {}
  };

  /** Binary operator. */
  class BinaryOp : public Instr {
  public:
    Instr* arg0;
    Instr* arg1;

    BinaryOp(Kind kind, Location location, Type* type, Instr* arg0, Instr* arg1)
      : Instr(kind, location, type)
      , arg0(arg0)
      , arg1(arg1)
    {}
  };

  /** Reference to an lvalue symbol, with optional stem expression. */
  class LValueRef : public Instr {
  public:
    Member* member;
    Instr* stem;

    LValueRef(
        Location location,
        Member* member,
        Instr* stem = nullptr,
        Type* type = nullptr)
      : Instr(Kind::LVALUE_REF, location, type)
      , member(member)
      , stem(stem)
    {}

    /** Dynamic casting support. */
    static bool classof(const LValueRef* e) { return true; }
    static bool classof(const Instr* e) { return e->kind == Kind::LVALUE_REF; }
  };
}

#endif
