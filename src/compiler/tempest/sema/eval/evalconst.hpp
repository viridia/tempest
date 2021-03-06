#ifndef TEMPEST_SEMA_GRAPH_EVAL_EVALCONST_HPP
#define TEMPEST_SEMA_GRAPH_EVAL_EVALCONST_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

namespace tempest::sema::eval {
  using tempest::sema::graph::Expr;
  using tempest::sema::graph::Type;

  struct EvalResult {
    enum DataType : uint8_t {
      INT,
      FLOAT,
      BOOL,
      STRING,
    };
    enum DataSize : uint8_t {
      F16,
      F32,
      F64,
      F128,
    };
    DataType type;  // Data type of the result
    DataSize size;  // Data size of the result (for floats)
    bool isUnsigned;// Whether the integer type is signed or unsigned
    bool hasSize;   // Whether the integer type has an explicit size (integer literals don't)

    // Flag that indicates we want to give up without an error if an argument
    bool failSilentIfNonConst = false;

    // Results of the evaluation
    llvm::APInt intResult;
    llvm::APFloat floatResult;
    std::string strResult;
    bool boolResult;
    bool error = false;

    EvalResult()
      : floatResult(llvm::APFloat::IEEEdouble())
      , boolResult(false)
    {}
  };

  bool evalConstExpr(const Expr* e, EvalResult& result);
}

#endif
