#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/error/diagnostics.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Casting.h"

namespace tempest::sema::graph {
  using tempest::error::diag;

  struct Formatter {
    ::std::ostream& out;
    int32_t indent = 0;
    bool pretty = false;
    bool showContent = true;

    Formatter(::std::ostream& out, bool pretty) : out(out), pretty(pretty) {}
    void visit(const Member* node);
    void visitType(const Type* t);
    void visitList(const DefnArray nodes);
    void visitNamedList(const DefnArray nodes, llvm::StringRef name);
    // void visitNamedNode(const Node* node, llvm::StringRef name);
    // void visitDefnFlags(const Defn* de);
    // void printFlag(llvm::StringRef name, bool enabled);
  };

  void Formatter::visit(const Member* m) {
    if (m == nullptr) {
      out << "<null>";
      return;
    }
    switch (m->kind) {
      case Member::Kind::TYPE: {
        auto td = static_cast<const TypeDefn*>(m);
        switch (td->type()->kind) {
          case Type::Kind::CLASS:
            out << "class";
            break;
          case Type::Kind::STRUCT:
            out << "struct";
            break;
          case Type::Kind::INTERFACE:
            out << "interface";
            break;
          case Type::Kind::TRAIT:
            out << "trait";
            break;
          default:
            out << "<invalid>";
            break;
        }
        out << ' ' << td->name();
        if (showContent) {
          out << " {";
          visitList(td->members());
          if (pretty) {
            out << '\n' << std::string(indent, ' ');
          }
          out << "}";
        }
        break;
      }

      case Member::Kind::TYPE_PARAM: {
        assert(false && "Implement");
        break;
      }

      case Member::Kind::CONST_DEF:
      case Member::Kind::LET_DEF: {
        auto vd = static_cast<const ValueDefn*>(m);
        if (vd->visibility() == PRIVATE) {
          out << "private ";
        } else if (vd->visibility() == PROTECTED) {
          out << "protected ";
        }
        if (vd->kind == Member::Kind::CONST_DEF) {
          out << "const ";
        }
        out << vd->name();
        if (vd->type()) {
          out << ": ";
        }
        out << ";";
        break;
      }

      case Member::Kind::ENUM_VAL: {
        assert(false && "Implement");
        break;
      }

      case Member::Kind::FUNCTION: {
        auto fd = static_cast<const FunctionDefn*>(m);
        out << "fn " << fd->name();
        break;
      }

      case Member::Kind::FUNCTION_PARAM: {
        assert(false && "Implement");
        break;
      }

      case Member::Kind::MODULE: {
        auto mod = static_cast<const Module*>(m);
        out << "module " << mod->name() << " {";
        visitList(mod->members());
        if (pretty) {
          out << '\n' << std::string(indent, ' ');
        }
        out << "}";
        break;
      }

      case Member::Kind::SPECIALIZED: {
        auto spec = static_cast<const SpecializedDefn*>(m);
        // TODO: We really need a separate function for type signatures.
        bool saveShowContent = showContent;
        showContent = false;
        visit(spec->generic());
        saveShowContent = true;
        out << "[";
        llvm::StringRef sep = "";
        for (auto member : spec->typeArgs()) {
          out << sep;
          sep = ", ";
          visitType(member);
        }
        out << "]";
        break;
      }

      // TUPLE_MEMBER,

      // COUNT,
      default:
        diag.error() << "Format not implemented: " << int(m->kind);
        assert(false && "Format not implemented.");
    }
  }

  void Formatter::visitType(const Type* t) {
    if (t == nullptr) {
      out << "<null>";
      return;
    }
    switch (t->kind) {
      case Type::Kind::INVALID:
        out << "<error>";
        break;

      case Type::Kind::NEVER:
        out << "<never>";
        break;

      case Type::Kind::IGNORED:
        out << "<ignored>";
        break;

      case Type::Kind::VOID:
        out << "void";
        break;

      case Type::Kind::BOOLEAN:
        out << "bool";
        break;

      case Type::Kind::INTEGER: {
        auto intType = static_cast<const IntegerType*>(t);
        out << intType->defn()->name();
        break;
      }

      case Type::Kind::FLOAT: {
        auto floatType = static_cast<const FloatType*>(t);
        out << floatType->defn()->name();
        break;
      }

      case Type::Kind::CLASS: {
        auto udt = static_cast<const UserDefinedType*>(t);
        out << "class " << udt->defn()->name();
        break;
      }

      case Type::Kind::STRUCT: {
        auto udt = static_cast<const UserDefinedType*>(t);
        out << "struct " << udt->defn()->name();
        break;
      }

      case Type::Kind::INTERFACE: {
        auto udt = static_cast<const UserDefinedType*>(t);
        out << "interface " << udt->defn()->name();
        break;
      }

      case Type::Kind::TRAIT: {
        auto udt = static_cast<const UserDefinedType*>(t);
        out << "trait " << udt->defn()->name();
        break;
      }

      case Type::Kind::EXTENSION: {
        auto udt = static_cast<const UserDefinedType*>(t);
        out << "extend " << udt->defn()->name();
        break;
      }

      case Type::Kind::ENUM: {
        auto udt = static_cast<const UserDefinedType*>(t);
        out << "enum " << udt->defn()->name();
        break;
      }

      // ALIAS,          // An alias for another type

      case Type::Kind::TYPE_VAR: {
        auto tv = static_cast<const TypeVar*>(t);
        out << "enum " << tv->param->name();
        break;
      }

      case Type::Kind::UNION: {
        auto ut = static_cast<const UnionType*>(t);
        llvm::StringRef sep = "";
        for (auto member : ut->members) {
          out << sep;
          sep = " | ";
          visitType(member);
        }
        break;
      }

      case Type::Kind::TUPLE: {
        auto ut = static_cast<const TupleType*>(t);
        llvm::StringRef sep = "(";
        for (auto member : ut->members) {
          out << sep;
          sep = ", ";
          visitType(member);
        }
        out << ")";
        break;
      }

      case Type::Kind::FUNCTION: {
        auto ft = static_cast<const FunctionType*>(t);
        llvm::StringRef sep = "fn (";
        for (auto member : ft->paramTypes) {
          out << sep;
          sep = ", ";
          visitType(member);
        }
        out << ")";
        if (ft->returnType) {
          out << " -> ";
          visitType(ft->returnType);
        }
        break;
      }

      case Type::Kind::CONST: {
        auto ct = static_cast<const ConstType*>(t);
        out << "const ";
        visitType(ct->base());
        break;
      }

      case Type::Kind::SPECIALIZED: {
        auto st = static_cast<const SpecializedType*>(t);
        visit(st->spec);
        break;
      }

      case Type::Kind::SINGLETON: {
        auto st = static_cast<const SingletonType*>(t);
        if (auto intLit = llvm::dyn_cast<IntegerLiteral>(st->value)) {
          llvm::SmallString<16> str;
          intLit->value().toString(str, 10, !intLit->isUnsigned());
          out << str;
        } else if (auto strLit = llvm::dyn_cast<StringLiteral>(st->value)) {
          out << "\"" << strLit->value() << "\"";
        }
        break;
      }

      // SINGLETON,      // A type consisting of a single value

      default:
        diag.error() << "Format not implemented: " << int(t->kind);
        assert(false && "Format not implemented.");
    }
  }

  void Formatter::visitList(const DefnArray nodes) {
    if (pretty) {
      indent += 2;
      for (auto arg : nodes) {
        out << '\n' << std::string(indent, ' ');
        visit(arg);
      }
      indent -= 2;
    } else {
      for (auto arg : nodes) {
        out << ' ';
        visit(arg);
      }
    }
  }

  void Formatter::visitNamedList(const DefnArray nodes, llvm::StringRef name) {
    if (nodes.size() > 0) {
      if (pretty) {
        indent += 2;
        out << '\n' << std::string(indent, ' ') << "(" << name;
        visitList(nodes);
        out << ')';
        indent -= 2;
      } else {
        out << "(" << name;
        visitList(nodes);
        out << ')';
      }
    }
  }

  void format(::std::ostream& out, const Member* m, bool pretty) {
    Formatter fmt(out, pretty);
    fmt.visit(m);
    if (pretty) {
      out << '\n';
    }
  }

  void format(::std::ostream& out, const Type* t) {
    Formatter fmt(out, false);
    fmt.visitType(t);
  }
}
