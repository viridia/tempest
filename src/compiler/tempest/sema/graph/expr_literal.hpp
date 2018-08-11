#ifndef TEMPEST_SEMA_GRAPH_EXPR_LITERAL_HPP
#define TEMPEST_SEMA_GRAPH_EXPR_LITERAL_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_PRIMITIVETYPE_HPP
  #include "tempest/sema/graph/primitivetype.hpp"
#endif

namespace tempest::sema::graph {
  /** Boolean literal. */
  class BooleanLiteral : public Expr {
  public:
    BooleanLiteral(Location location, bool value)
      : Expr(Kind::BOOLEAN_LITERAL, location)
      , _value(value)
    {}

    /** The value of the boolean literal. */
    bool value() const { return _value; }

    /** Dynamic casting support. */
    static bool classof(const BooleanLiteral* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::BOOLEAN_LITERAL; }

  private:
    bool _value;
  };

  /** Integer literal. */
  class IntegerLiteral : public Expr {
  public:
    /** Whether this literal had an unsigned suffix when parsed. */
    bool isUnsigned = false;

    /** Whether this literal had an explicit negative sign when parsed. */
    bool isNegative = false;

    IntegerLiteral(
          Location location,
          const llvm::APInt& value,
          bool isUnsigned = false,
          bool isNegative = false,
          IntegerType* type = nullptr)
      : Expr(Kind::INTEGER_LITERAL, location)
      , isUnsigned(isUnsigned)
      , isNegative(isNegative)
      , _value(value)
    {
      this->type = type;
    }
    IntegerLiteral(int32_t value, IntegerType* type)
      : Expr(Kind::INTEGER_LITERAL, Location())
      , isUnsigned(false)
      , isNegative(false)
      , _value(llvm::APInt(32, value, true))
    {
      this->type = type;
    }

    /** The value as an arbitrary-precision integer. */
    llvm::APInt& value() { return _value; }
    const llvm::APInt& value() const { return _value; }

    /** Dynamic casting support. */
    static bool classof(const IntegerLiteral* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::INTEGER_LITERAL; }

  private:
    // TODO: This leaks member because we never call IntegerLiteral's destructor. We should
    // instead store the word array directly on the BumpPtrAllocator and synthesize an APInt
    // when needed.
    llvm::APInt _value;
  };

  /** Floating-point literal. */
  class FloatLiteral : public Expr {
  public:
    /** Whether this literal had an explicit negative sign when parsed. */
    bool isNegative = false;

    FloatLiteral(
        Location location,
        const llvm::APFloat &value,
        bool isNegative = false,
        FloatType* type = nullptr)
      : Expr(Kind::FLOAT_LITERAL, location)
      , isNegative(isNegative)
      , _value(value)
    {
      this->type = type;
    }

    /** The value as an arbitrary-precision integer. */
    llvm::APFloat& value() { return _value; }
    const llvm::APFloat& value() const { return _value; }

    /** Dynamic casting support. */
    static bool classof(const FloatLiteral* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::FLOAT_LITERAL; }

  private:
    llvm::APFloat _value;
  };

  /** String literal. */
  class StringLiteral : public Expr {
  public:
    StringLiteral(Location location, llvm::StringRef value)
      : Expr(Kind::STRING_LITERAL, location)
      , _value(value)
    {}

    /** The value as an arbitrary-precision integer. */
    llvm::StringRef& value() { return _value; }
    const llvm::StringRef& value() const { return _value; }

    /** Dynamic casting support. */
    static bool classof(const StringLiteral* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::STRING_LITERAL; }

  private:
    llvm::StringRef _value;
  };
}

#endif
