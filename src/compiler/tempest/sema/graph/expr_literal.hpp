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

  /** A way to store a multi-precision integer in a bump-pointer allocator. */
  typedef ArrayRef<llvm::APInt::WordType> MPInt;

  /** Integer literal. */
  class IntegerLiteral : public Expr {
  public:
    IntegerLiteral(
          Location location,
          const MPInt& value,
          IntegerType* type = nullptr)
      : Expr(Kind::INTEGER_LITERAL, location)
      , _value(value)
    {
      this->type = type;
    }
    IntegerLiteral(
          const MPInt& value,
          IntegerType* type = nullptr)
      : Expr(Kind::INTEGER_LITERAL, Location())
      , _value(value)
    {
      this->type = type;
    }

    /** Type downcast to integer type. */
    IntegerType* intType() const { return cast <IntegerType>(type); }

    /** The value as an arbitrary-precision integer. */
    MPInt value() { return _value; }
    const MPInt& value() const { return _value; }

    /** The value as an LLVM APInt. */
    const llvm::APInt asAPInt() const {
      return llvm::APInt(intType()->bits(), _value);
    }

    /** Dynamic casting support. */
    static bool classof(const IntegerLiteral* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::INTEGER_LITERAL; }

  private:
    MPInt _value;
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
