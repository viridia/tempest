#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/type.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/gen/linkagename.hpp"
#include <llvm/Support/Casting.h>
#include <assert.h>

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using tempest::error::diag;
  using llvm::dyn_cast;

  void getDefnLinkageName(std::string& out, const Member* m, llvm::ArrayRef<const Type*> typeArgs) {
    assert(m != nullptr);
    auto defn = llvm::dyn_cast<const Defn>(m);
    if (defn && defn->definedIn()) {
      if (defn->definedIn()->kind == Member::Kind::MODULE) {
        auto mod = static_cast<const Module*>(defn->definedIn());
        out.append(mod->name().begin(), mod->name().end());
        out.push_back('.');
      } else {
        getDefnLinkageName(out, defn->definedIn(), typeArgs);
        out.push_back('.');
      }
    }

    switch (m->kind) {
      case Member::Kind::TYPE: {
        auto td = static_cast<const TypeDefn*>(m);
        out.append(td->name());
        break;
      }

      case Member::Kind::SPECIALIZED: {
        auto sd = static_cast<const SpecializedDefn*>(m);
        getDefnLinkageName(out, sd->generic(), typeArgs);
        out.push_back('[');
        char sep = 0;
        for (auto param : sd->typeParams()) {
          if (sep) {
            out.push_back(sep);
          }
          sep = ',';
          const Type* arg = sd->typeArgs()[param->index()];
          getTypeLinkageName(out, arg, typeArgs);
        }
        out.push_back(']');
        break;
      }

      case Member::Kind::FUNCTION: {
        auto fd = static_cast<const FunctionDefn*>(m);
        out.append(fd->name());
        getTypeLinkageName(out, fd->type(), typeArgs);
        break;
      }

      // TYPE_PARAM,
      // CONST,
      // VAR,
      // ENUM_VAL,
      // FUNCTION_PARAM,

      // MODULE,

      case Member::Kind::MODULE: {
        diag.fatal() << "Attempt to generate linkage name for module: " << m->name();
        break;
      }

      default:
        diag.error() << "Bad member kind: " << m;
        assert(false && "Unsupported member kind");
    }
  }

  void getTypeLinkageName(std::string& out, const Type* ty, llvm::ArrayRef<const Type*> typeArgs) {
    assert(ty != nullptr);
    switch (ty->kind) {
      case Type::Kind::VOID: {
        out.append("void");
        break;
      }

      case Type::Kind::BOOLEAN: {
        out.append("bool");
        break;
      }

      case Type::Kind::INTEGER: {
        auto intTy = static_cast<const IntegerType*>(ty);
        out.append(intTy->defn()->name());
        break;
      }

      case Type::Kind::FLOAT: {
        auto floatTy = static_cast<const FloatType*>(ty);
        out.append(floatTy->defn()->name());
        break;
      }

      case Type::Kind::TUPLE: {
        auto tupleTy = static_cast<const TupleType*>(ty);
        out.push_back('(');
        char sep = 0;
        for (auto member : tupleTy->members) {
          if (sep) {
            out.push_back(sep);
          }
          sep = ',';
          getTypeLinkageName(out, member, typeArgs);
        }
        out.push_back(')');
        break;
      }

      case Type::Kind::UNION: {
        auto unionTy = static_cast<const UnionType*>(ty);
        char sep = 0;
        for (auto member : unionTy->members) {
          if (sep) {
            out.push_back(sep);
          }
          sep = '|';
          getTypeLinkageName(out, member, typeArgs);
        }
        break;
      }

      case Type::Kind::FUNCTION: {
        auto funcTy = static_cast<const FunctionType*>(ty);
        char sep = 0;
        if (funcTy->paramTypes.size() > 0) {
          out.push_back('(');
          for (auto member : funcTy->paramTypes) {
            if (sep) {
              out.push_back(sep);
            }
            sep = ',';
            getTypeLinkageName(out, member, typeArgs);
          }
          out.push_back(')');
        }
        if (funcTy->returnType->kind != Type::Kind::VOID) {
          out.append("->");
          getTypeLinkageName(out, funcTy->returnType, typeArgs);
        }
        break;
      }

      case Type::Kind::CLASS: {
        auto udt = static_cast<const UserDefinedType*>(ty);
        getDefnLinkageName(out, udt->defn(), typeArgs);
        break;
      }

      default:
        diag.fatal() << "Unsupported type kind: " << ty->kind;
        assert(false && "Unsupported type kind");
    }
  }
}
