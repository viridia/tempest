#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/type.hpp"
#include "tempest/error/diagnostics.hpp"

namespace tempest::sema::graph {
  using tempest::error::diag;

  struct Formatter {
    ::std::ostream& out;
    int32_t indent = 0;
    bool pretty = false;

    Formatter(::std::ostream& out, bool pretty) : out(out), pretty(pretty) {}
    void visit(const Member* node);
    void visitList(const std::vector<Defn*>& nodes);
    void visitNamedList(const std::vector<Defn*>& nodes, llvm::StringRef name);
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
        out << " {";
        visitList(td->members());
        if (pretty) {
          out << '\n' << std::string(indent, ' ');
        }
        out << "}";
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
        assert(false && "Implement");
        break;
      }

      // TUPLE_MEMBER,

      // COUNT,
      default:
        diag.error() << "Format not implemented: " << int(m->kind);
        assert(false && "Format not implemented.");
    }
  }

  void Formatter::visitList(const std::vector<Defn*>& nodes) {
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

  void Formatter::visitNamedList(const std::vector<Defn*>& nodes, llvm::StringRef name) {
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
}
