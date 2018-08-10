#ifndef TEMPEST_SEMA_GRAPH_EXPR_LITERAL_HPP
#define TEMPEST_SEMA_GRAPH_EXPR_LITERAL_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

namespace tempest::sema::graph {
  class IntegerType;
  class FloatType;

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
    IntegerLiteral(
          Location location,
          const llvm::APInt& value,
          bool isUnsigned = false,
          bool isNegative = false,
          IntegerType* type = nullptr)
      : Expr(Kind::INTEGER_LITERAL, location)
      , _value(value)
      , _isUnsigned(isUnsigned)
      , _isNegative(isNegative)
      , _type(type)
    {}
    IntegerLiteral(int32_t value, IntegerType* type)
      : Expr(Kind::INTEGER_LITERAL, Location())
      , _value(llvm::APInt(32, value, true))
      , _isUnsigned(false)
      , _isNegative(false)
      , _type(type)
    {}

    /** Type of the integer */
    IntegerType* type() const { return _type; }
    void setType(IntegerType* type) { _type = type; }

    /** The value as an arbitrary-precision integer. */
    llvm::APInt& value() { return _value; }
    const llvm::APInt& value() const { return _value; }

    /** Whether this literal had an explicit negative sign when parsed. */
    bool isNegative() const { return _isNegative; }
    void setNegative(bool negative) { _isNegative = negative; }

    /** Whether this literal was explicitly unsigned. */
    bool isUnsigned() const { return _isUnsigned; }
    void setUnsigned(bool uns) { _isUnsigned = uns; }

    /** Dynamic casting support. */
    static bool classof(const IntegerLiteral* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::INTEGER_LITERAL; }

  private:
    llvm::APInt _value;
    bool _isUnsigned = false;
    bool _isNegative = false;
    IntegerType* _type = nullptr;
  };

  /** Floating-point literal. */
  class FloatLiteral : public Expr {
  public:
    FloatLiteral(
        Location location,
        const llvm::APFloat &value,
        bool isNegative = false,
        FloatType* type = nullptr)
      : Expr(Kind::FLOAT_LITERAL, location)
      , _value(value)
      , _isNegative(isNegative)
      , _type(type)
    {}

    /** Type of the float */
    FloatType* type() const { return _type; }
    void setType(FloatType* type) { _type = type; }

    /** The value as an arbitrary-precision integer. */
    llvm::APFloat& value() { return _value; }
    const llvm::APFloat& value() const { return _value; }

    /** Whether this literal had an explicit negative sign when parsed. */
    bool isNegative() const { return _isNegative; }
    void setNegative(bool negative) { _isNegative = negative; }

    /** Dynamic casting support. */
    static bool classof(const FloatLiteral* e) { return true; }
    static bool classof(const Expr* e) { return e->kind == Kind::FLOAT_LITERAL; }

  private:
    llvm::APFloat _value;
    bool _isNegative = false;
    FloatType* _type = nullptr;
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
