#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_lowered.hpp"
#include "tempest/sema/graph/expr_op.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/infer/overload.hpp"
#include "tempest/sema/infer/types.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/gen/outputsym.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Casting.h"

namespace tempest::sema::graph {
  using tempest::error::diag;
  using tempest::sema::infer::ContingentType;
  using tempest::sema::infer::InferredType;

  struct Formatter {
    ::std::ostream& out;
    int32_t indent = 0;
    bool pretty = false;
    bool showType = false;
    bool showContent = true;

    Formatter(::std::ostream& out, bool pretty) : out(out), pretty(pretty) {}
    void visit(const Member* node);
    void visitType(const Type* t);
    void visitList(const DefnArray nodes);
    void visitNamedList(const DefnArray nodes, llvm::StringRef name);
    void visitExpr(const Expr* e);
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
          case Type::Kind::ALIAS:
            out << "type";
            break;
          case Type::Kind::EXTENSION:
            out << "extend";
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
        auto tp = static_cast<const TypeParameter*>(m);
        out << tp->name();
        break;
      }

      case Member::Kind::VAR_DEF: {
        auto vd = static_cast<const ValueDefn*>(m);
        if (vd->visibility() == PRIVATE) {
          out << "private ";
        } else if (vd->visibility() == PROTECTED) {
          out << "protected ";
        }
        if (vd->isConstant()) {
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
        if (showType && fd->type()) {
          if (fd->typeParams().size() > 0) {
            out << "[";
            auto sep = "";
            for (auto p : fd->typeParams()) {
              out << sep;
              sep = ", ";
              out << p->name();
              if (p->subtypeConstraints().size() > 0) {
                out << ": ";
                auto sep2 = "";
                for (auto st : p->subtypeConstraints()) {
                  out << sep2;
                  sep2 = " & ";
                  out << st;
                }
              }
            }
            out << "]";
          }
          out << "(";
          auto sep = "";
          for (auto p : fd->params()) {
            out << sep;
            sep = ", ";
            out << p->name() << ": " << p->type();
          }
          if (fd->isVariadic()) {
            out << "...";
          }
          out << ") -> ";
          out << fd->type()->returnType;
        }
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

      case Type::Kind::NOT_EXPR:
        out << "<not-expr>";
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

      case Type::Kind::ALIAS: {
        auto aliasType = static_cast<const UserDefinedType*>(t);
        out << "type " << aliasType->defn()->name();
        break;
      }

      case Type::Kind::TYPE_VAR: {
        auto tv = static_cast<const TypeVar*>(t);
        if (showType) {
          out << "type parameter ";
        }
        out << tv->param->name();
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
        out << "fn (";
        llvm::StringRef sep = "";
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

      case Type::Kind::MODIFIED: {
        auto ct = static_cast<const ModifiedType*>(t);
        if (ct->isReadOnly()) {
          out << "const ";
        }
        if (ct->isImmutable()) {
          out << "immutable ";
        }
        visitType(ct->base);
        break;
      }

      case Type::Kind::SPECIALIZED: {
        auto st = static_cast<const SpecializedType*>(t);
        visit(st->spec);
        break;
      }

      case Type::Kind::SINGLETON: {
        auto st = static_cast<const SingletonType*>(t);
        visitExpr(st->value);
        break;
      }

      case Type::Kind::CONTINGENT: {
        auto ct = static_cast<const ContingentType*>(t);
        out << "any of (";
        for (size_t i = 0; i < ct->entries.size(); i += 1) {
          auto& member = ct->entries[i];
          if (i > 0) {
            if (i == ct->entries.size() - 1) {
              out << " or ";
            } else {
              out << ", ";
            }
          }
          if (!member.when->isViable()) {
            out << "-";
          }
          visitType(member.type);
        }
        out << ")";
        break;
      }

      case Type::Kind::INFERRED: {
        auto it = static_cast<const InferredType*>(t);
        out << "inferred " << it->typeParam->name();
        break;
      }

      default:
        diag.error() << "Format not implemented: " << t->kind;
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

  void Formatter::visitExpr(const Expr* e) {
    if (e == nullptr) {
      out << "<null>";
      return;
    }
    switch (e->kind) {
      case Expr::Kind::INVALID:
        out << "<error>";
        break;

      case Expr::Kind::BOOLEAN_LITERAL: {
        auto boolLit = static_cast<const BooleanLiteral*>(e);
        out << (boolLit->value() ? "true" : "false");
        break;
      }

      case Expr::Kind::INTEGER_LITERAL: {
        auto intLit = static_cast<const IntegerLiteral*>(e);
        llvm::SmallString<16> str;
        intLit->asAPInt().toString(str, 10, !intLit->intType()->isUnsigned());
        out << str;
        break;
      }

      case Expr::Kind::STRING_LITERAL: {
        auto strLit = static_cast<const StringLiteral*>(e);
        out << "\"" << strLit->value() << "\"";
        break;
      }

      case Expr::Kind::FLOAT_LITERAL: {
        auto floatLit = static_cast<const FloatLiteral*>(e);
        llvm::SmallString<16> str;
        floatLit->value().toString(str, 5);
        out << str;
        break;
      }

      case Expr::Kind::SELF:
        out << "self";
        break;

      case Expr::Kind::FUNCTION_REF:
      case Expr::Kind::TYPE_REF:
      case Expr::Kind::VAR_REF: {
        auto fnRef = static_cast<const DefnRef*>(e);
        if (fnRef->stem) {
          visitExpr(fnRef->stem);
          out << ".";
        }
        out << fnRef->defn->name();
        break;
      }

      case Expr::Kind::FUNCTION_REF_OVERLOAD: {
        auto fnRef = static_cast<const MemberListExpr*>(e);
        out << fnRef->members.front().member->name();
        if (fnRef->members.size() > 1) {
          out << "×" << fnRef->members.size();
        }
        break;
      }

      case Expr::Kind::ASSIGN: {
        auto op = static_cast<const BinaryOp*>(e);
        out << "(";
        visitExpr(op->args[0]);
        out << " = ";
        visitExpr(op->args[1]);
        out << ")";
        break;
      }

      case Expr::Kind::ADD: {
        auto op = static_cast<const BinaryOp*>(e);
        out << "(";
        visitExpr(op->args[0]);
        out << " + ";
        visitExpr(op->args[1]);
        out << ")";
        break;
      }

      case Expr::Kind::CALL: {
        auto op = static_cast<const ApplyFnOp*>(e);
        visitExpr(op->function);
        out << "(";
        llvm::StringRef sep = "";
        for (auto arg : op->args) {
          out << sep;
          sep = ", ";
          visitExpr(arg);
        }
        out << ")";
        break;
      }

      case Expr::Kind::REST_ARGS: {
        auto op = static_cast<const MultiOp*>(e);
        out << "[";
        llvm::StringRef sep = "";
        for (auto arg : op->args) {
          out << sep;
          sep = ", ";
          visitExpr(arg);
        }
        out << "]";
        break;
      }

      case Expr::Kind::BLOCK: {
        auto blk = static_cast<const BlockStmt*>(e);
        out << "{";
        if (pretty) {
          indent += 2;
          for (auto st : blk->stmts) {
            out << '\n' << std::string(indent, ' ');
            visitExpr(st);
          }
          if (blk->result) {
            out << '\n' << std::string(indent, ' ');
            visitExpr(blk->result);
          }
          indent -= 2;
          out << '\n' << std::string(indent, ' ') << "}";
        } else {
          out << " ";
          for (auto st : blk->stmts) {
            visitExpr(st);
          }
          if (blk->result) {
            visitExpr(blk->result);
          }
          out << " }";
        }
        break;
      }

      case Expr::Kind::LOCAL_VAR: {
        auto lvar = static_cast<const LocalVarStmt*>(e);
        out << lvar->defn;
        break;
      }

      case Expr::Kind::CAST_SIGN_EXTEND: {
        auto op = static_cast<const UnaryOp*>(e);
        out << "(sext ";
        visitExpr(op->arg);
        out << ")";
        break;
      }

      case Expr::Kind::CAST_ZERO_EXTEND: {
        auto op = static_cast<const UnaryOp*>(e);
        out << "(zext ";
        visitExpr(op->arg);
        out << ")";
        break;
      }

      case Expr::Kind::CAST_INT_TRUNCATE: {
        auto op = static_cast<const UnaryOp*>(e);
        out << "(trunc ";
        visitExpr(op->arg);
        out << ")";
        break;
      }

      case Expr::Kind::CAST_FP_EXTEND: {
        auto op = static_cast<const UnaryOp*>(e);
        out << "(fext ";
        visitExpr(op->arg);
        out << ")";
        break;
      }

      case Expr::Kind::CAST_FP_TRUNC: {
        auto op = static_cast<const UnaryOp*>(e);
        out << "(ftrunc ";
        visitExpr(op->arg);
        out << ")";
        break;
      }

      case Expr::Kind::RETURN: {
        auto op = static_cast<const UnaryOp*>(e);
        out << "(return ";
        visitExpr(op->arg);
        out << ")";
        break;
      }

      case Expr::Kind::ALLOC_OBJ:
      case Expr::Kind::GLOBAL_REF: {
        auto op = static_cast<const SymbolRefExpr*>(e);
        out << (e->kind == Expr::Kind::ALLOC_OBJ ? "(alloc" : "(global");
        switch (op->sym->kind) {
          case gen::OutputSym::Kind::FUNCTION: {
            out << " fn";
            break;
          }
          case gen::OutputSym::Kind::CLS_DESC: {
            out << " cls_desc";
            break;
          }
          default:
            break;
        }
        if (op->stem) {
          out << " ";
          visitExpr(op->stem);
        }
        out << ")";
        break;
      }

      default:
        diag.error() << "Format not implemented: " << Expr::KindName(e->kind);
        assert(false && "Format not implemented.");
    }
  }

  void format(::std::ostream& out, const Member* m, bool pretty, bool showType) {
    Formatter fmt(out, pretty);
    fmt.pretty = pretty;
    fmt.showType = showType;
    fmt.visit(m);
    if (pretty) {
      out << '\n';
    }
  }

  void format(::std::ostream& out, const Type* t) {
    Formatter fmt(out, false);
    fmt.visitType(t);
  }

  void format(::std::ostream& out, const Expr* e) {
    Formatter fmt(out, false);
    fmt.visitExpr(e);
  }
}
