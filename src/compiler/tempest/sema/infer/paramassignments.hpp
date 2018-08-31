#ifndef TEMPEST_SEMA_INFER_PARAMASSIGNMENTS_HPP
#define TEMPEST_SEMA_INFER_PARAMASSIGNMENTS_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_EXPR_HPP
  #include "tempest/sema/graph/expr.hpp"
#endif

#ifndef TEMPEST_SEMA_INFER_REJECTION_HPP
  #include "tempest/sema/infer/rejection.hpp"
#endif

#include <unordered_set>

namespace tempest::sema::infer {
  using namespace tempest::sema::graph;

  enum class ParamError {
    NONE,
    POSITIONAL_AFTER_KEYWORD,
    TOO_MANY_ARGUMENTS,
    KEYWORD_ONLY_PARAM,
    KEYWORD_NOT_FOUND,
    KEYWORD_IN_USE,
    NOT_ENOUGH_ARGS,
  };

  /** Decides for a function call which argument is bound to which parameter. Handles default
      arguments, keyword arguments, etc. */
  class ParameterAssignmentsBuilder {
  public:
    ParameterAssignmentsBuilder(
        llvm::SmallVectorImpl<size_t>& output,
        const llvm::ArrayRef<ParameterDefn*> &params,
        bool isVariadic)
      : _output(output)
      , _params(params)
      , _isVariadic(isVariadic)
    {
      _inUse.resize(_params.size());
    }

    ParamError error() const { return _error; }

    Rejection rejection() {
      // TODO: Include the argument position and keyword name in the rejection.
      switch (_error) {
        case ParamError::NONE:
          return Rejection(Rejection::NONE);

        case ParamError::NOT_ENOUGH_ARGS:
          return Rejection(Rejection::NOT_ENOUGH_ARGS);

        case ParamError::TOO_MANY_ARGUMENTS:
          return Rejection(Rejection::TOO_MANY_ARGS);

        case ParamError::KEYWORD_ONLY_PARAM:
          return Rejection(Rejection::KEYWORD_ONLY_ARG);

        case ParamError::KEYWORD_IN_USE:
          return Rejection(Rejection::KEYWORD_IN_USE);

        case ParamError::KEYWORD_NOT_FOUND:
          return Rejection(Rejection::KEYWORD_NOT_FOUND);

        default:
          assert(false && "Invalid error state");
      }
    }

    ParamError addPositionalArg() {
      if (_error != ParamError::NONE) {
        return _error;
      }

      if (_keywordsAdded) {
        _error = ParamError::POSITIONAL_AFTER_KEYWORD;
        return _error;
      }

      if (_nextPositionalParam >= _params.size()) {
        _error = ParamError::TOO_MANY_ARGUMENTS;
        return _error;
      }

      auto param = _params[_nextPositionalParam];
      if (param->isKeywordOnly()) {
        _error = ParamError::KEYWORD_ONLY_PARAM;
        return _error;
      }

      _output.push_back(_nextPositionalParam);
      _inUse[_nextPositionalParam] = true;

      if (!_isVariadic || _nextPositionalParam < _params.size() - 1) {
        _nextPositionalParam += 1;
      }

      return ParamError::NONE;
    }

    ParamError addKeywordArg(const llvm::StringRef& name) {
      if (_error != ParamError::NONE) {
        return _error;
      }

      size_t paramIndex = 0;
      for (; paramIndex < _params.size(); ++paramIndex) {
        auto param = _params[paramIndex];
        if (param->name() == name) {
          break;
        }
      }

      if (paramIndex >= _params.size()) {
        _error = ParamError::KEYWORD_NOT_FOUND;
        return _error;
      }

      if (_inUse[_nextPositionalParam]) {
        _error = ParamError::KEYWORD_IN_USE;
        return _error;
      }

      _output.push_back(paramIndex);
      _inUse[paramIndex] = true;
      _keywordsAdded = true;
      return ParamError::NONE;
    }

    ParamError validate() {
      if (_error != ParamError::NONE) {
        return _error;
      }

      size_t numParams = _isVariadic ? _params.size() - 1 : _params.size();
      for (size_t paramIndex = 0; paramIndex < numParams; ++paramIndex) {
        if (!_inUse[paramIndex]) {
          auto param = _params[paramIndex];
          if (!param->init()) {
            _error = ParamError::NOT_ENOUGH_ARGS;
            return _error;
          }
        }
      }

      return ParamError::NONE;
    }

  private:
    llvm::SmallVectorImpl<size_t>& _output;
    const llvm::ArrayRef<ParameterDefn*> &_params;
    llvm::SmallVector<size_t, 16> _inUse;
    size_t _nextPositionalParam = 0;
    ParamError _error = ParamError::NONE;
    bool _keywordsAdded = false;
    bool _isVariadic = false;
  };

  /** Similar to the above when all we have is a function type.
      Note that a callable type signature can include variadic parameters, it does not include
      keyword arguments or default parameters which are properties of a function, not a function
      type. That means that these features are not available when calling a function indirectly.
      (It also means that functions with the same parameter types but having different keywords
      have compatible function types.)
  */
  class ParameterAssignmentsTypeBuilder {
  public:
    ParameterAssignmentsTypeBuilder(
        llvm::SmallVectorImpl<size_t>& output,
        const FunctionType* ftype)
      : _output(output)
      , _ftype(ftype)
    {
      _inUse.resize(_ftype->paramTypes.size());
    }

    ParamError addPositionalArg() {
      if (_error != ParamError::NONE) {
        return _error;
      }

      if (_nextPositionalParam >= _ftype->paramTypes.size()) {
        _error = ParamError::TOO_MANY_ARGUMENTS;
        return _error;
      }

      _output.push_back(_nextPositionalParam);
      _inUse[_nextPositionalParam] = true;

      if (!isVariadic(_nextPositionalParam)) {
        _nextPositionalParam += 1;
      }

      return ParamError::NONE;
    }

    ParamError validate() {
      if (_error != ParamError::NONE) {
        return _error;
      }

      for (size_t paramIndex = 0; paramIndex < _ftype->paramTypes.size(); ++paramIndex) {
        if (!_inUse[paramIndex] && !isVariadic(paramIndex)) {
          _error = ParamError::NOT_ENOUGH_ARGS;
          return _error;
        }
      }

      return ParamError::NONE;
    }

  private:
    bool isVariadic(size_t index) {
      return _ftype->isVariadic && index == _ftype->paramTypes.size() - 1;
    }

    llvm::SmallVectorImpl<size_t>& _output;
    const FunctionType* _ftype;
    llvm::SmallVector<size_t, 16> _inUse;
    size_t _nextPositionalParam = 0;
    ParamError _error = ParamError::NONE;
  };
}

#endif
