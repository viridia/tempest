#include "tempest/sema/graph/defn.h"
#include "tempest/sema/graph/type.h"
#include "tempest/sema/graph/primitivetype.h"
#include "tempest/gen/linkagename.h"
#include <llvm/Support/Casting.h>
#include <assert.h>

namespace tempest::gen {
  using namespace tempest::sema::graph;
  using llvm::dyn_cast;

  void getDefnLinkageName(std::string& out, const Member* m, llvm::ArrayRef<Type*> typeArgs) {
    assert(m != nullptr);
    auto defn = llvm::dyn_cast<const Defn>(m);
    if (defn && defn->definedIn()) {
      getDefnLinkageName(out, defn->definedIn(), typeArgs);
      out.push_back('.');
    }

    switch (m->kind) {
      case Member::Kind::TYPE: {
        auto td = static_cast<const TypeDefn*>(m);
        out.append(td->name());
        break;
      }
 
      case Member::Kind::SPECIALIZED: {
        auto sd = static_cast<const SpecializedDefn*>(m);
        auto base = sd->base();
        getDefnLinkageName(out, sd->base(), typeArgs);
        // assert(base->typeParams().size() == sd->typeArgs().size());
        out.push_back('[');
        char sep = 0;
        for (auto param : base->typeParams()) {
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
      // PACKAGE,


      default:
        assert(false && "Unsupported member kind");
    }
  }

  void getTypeLinkageName(std::string& out, const Type* ty, llvm::ArrayRef<Type*> typeArgs) {
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
        out.push_back('(');
        for (auto member : funcTy->paramTypes) {
          if (sep) {
            out.push_back(sep);
          }
          sep = ',';
          getTypeLinkageName(out, member, typeArgs);
        }
        out.push_back(')');
        out.append("->");
        getTypeLinkageName(out, funcTy->returnType, typeArgs);
        break;
      }

      default:
        assert(false && "Unsupported type kind");
    }
  }
}
