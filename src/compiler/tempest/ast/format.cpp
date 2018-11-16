#include "tempest/ast/node.hpp"
#include "tempest/ast/defn.hpp"
#include "tempest/ast/ident.hpp"
#include "tempest/ast/literal.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/ast/oper.hpp"
#include "tempest/error/diagnostics.hpp"

namespace tempest::ast {
  using tempest::error::diag;

  struct Formatter {
    ::std::ostream& out;
    int32_t indent = 0;
    bool pretty = false;

    Formatter(::std::ostream& out, bool pretty) : out(out), pretty(pretty) {}
    void visit(const Node* node);
    void visitList(const NodeList& nodes);
    void visitNamedList(const NodeList& nodes, llvm::StringRef name);
    void visitNamedNode(const Node* node, llvm::StringRef name);
    void visitDefnFlags(const Defn* de);
    void printFlag(llvm::StringRef name, bool enabled);
  };

  void Formatter::visit(const Node* node) {
    if (node == nullptr) {
      out << "<null>";
      return;
    }
    switch (node->kind) {
      case Node::Kind::ERROR:
        out << "#ERROR";
        break;

      case Node::Kind::ABSENT:
        out << "#ABSENT";
        break;

      case Node::Kind::NULL_LITERAL:
        out << "#NULL";
        break;

      case Node::Kind::SELF:
        out << "#SELF";
        break;

      case Node::Kind::SUPER:
        out << "#SUPER";
        break;

      case Node::Kind::IDENT: {
        auto ident = static_cast<const Ident*>(node);
        out << ident->name;
        break;
      }

      case Node::Kind::MEMBER: {
        auto m = static_cast<const MemberRef*>(node);
        visit(m->base);
        out << '.' << m->name;
        break;
      }

      case Node::Kind::SELF_NAME_REF: {
        auto ident = static_cast<const Ident*>(node);
        out << '.' << ident->name;
        break;
      }

      case Node::Kind::BUILTIN_ATTRIBUTE: {
        auto ba = static_cast<const BuiltinAttribute*>(node);
        switch (ba->attribute) {
          case BuiltinAttribute::INTRINSIC:
            out << "__intrinsic__";
            break;
          case BuiltinAttribute::TRACEMETHOD:
            out << "__tracemethod__";
            break;
        }
        break;
      }

      case Node::Kind::BUILTIN_TYPE: {
        auto bt = static_cast<const BuiltinType*>(node);
        switch (bt->type) {
          case BuiltinType::VOID: out << "void"; break;
          case BuiltinType::BOOL: out << "bool"; break;
          case BuiltinType::CHAR: out << "char"; break;
          case BuiltinType::I8: out << "i8"; break;
          case BuiltinType::I16: out << "i16"; break;
          case BuiltinType::I32: out << "i32"; break;
          case BuiltinType::I64: out << "i64"; break;
          case BuiltinType::INT: out << "int"; break;
          case BuiltinType::U8: out << "u8"; break;
          case BuiltinType::U16: out << "u16"; break;
          case BuiltinType::U32: out << "u32"; break;
          case BuiltinType::U64: out << "u64"; break;
          case BuiltinType::UINT: out << "uint"; break;
          case BuiltinType::F32: out << "f32"; break;
          case BuiltinType::F64: out << "f64"; break;
        }
        break;
      }

      case Node::Kind::KEYWORD_ARG: {
        auto ka = static_cast<const KeywordArg*>(node);
        out << ka->name << " = ";
        visit(ka->arg);
        break;
      }

      case Node::Kind::BOOLEAN_TRUE:
        out << "true";
        break;

      case Node::Kind::BOOLEAN_FALSE:
        out << "false";
        break;

      case Node::Kind::CHAR_LITERAL: {
        auto l = static_cast<const Literal*>(node);
        // TODO: Escape.
        out << '\'' << l->value << '\'';
        break;
      }

      case Node::Kind::INTEGER_LITERAL: {
        auto l = static_cast<const Literal*>(node);
        out << "(int " << l->value << l->suffix << ")";
        break;
      }

      case Node::Kind::FLOAT_LITERAL: {
        auto l = static_cast<const Literal*>(node);
        out << "(float " << l->value << l->suffix << ")";
        break;
      }

      case Node::Kind::STRING_LITERAL: {
        auto l = static_cast<const Literal*>(node);
        // TODO: Escape.
        out << '\"' << l->value << '\"';
        break;
      }

      case Node::Kind::NEGATE:
      case Node::Kind::COMPLEMENT:
      case Node::Kind::LOGICAL_NOT:
      case Node::Kind::ARRAY_TYPE: {
        auto op = static_cast<const UnaryOp*>(node);
        out << "(#" << Node::KindName(op->kind) << ' ';
        visit(op->arg);
        out << ')';
        break;
      }

      // case Node::Kind::PRE_INC: return "PRE_INC";
      // case Node::Kind::POST_INC: return "POST_INC";
      // case Node::Kind::PRE_DEC: return "PRE_DEC";
      // case Node::Kind::POST_DEC: return "POST_DEC";
      // case Node::Kind::PROVISIONAL_CONST: return "PROVISIONAL_CONST";
      // case Node::Kind::OPTIONAL: return "OPTIONAL";
      case Node::Kind::ADD:
      case Node::Kind::SUB:
      case Node::Kind::MUL:
      case Node::Kind::DIV:
      case Node::Kind::MOD:
      case Node::Kind::BIT_AND:
      case Node::Kind::BIT_OR:
      case Node::Kind::BIT_XOR:
      case Node::Kind::RSHIFT:
      case Node::Kind::LSHIFT:
      case Node::Kind::EQUAL:
      case Node::Kind::REF_EQUAL:
      case Node::Kind::NOT_EQUAL:
      case Node::Kind::LESS_THAN:
      case Node::Kind::GREATER_THAN:
      case Node::Kind::LESS_THAN_OR_EQUAL:
      case Node::Kind::GREATER_THAN_OR_EQUAL:
      case Node::Kind::IS_SUB_TYPE:
      case Node::Kind::IS_SUPER_TYPE:
      case Node::Kind::ASSIGN:
      case Node::Kind::ASSIGN_ADD:
      case Node::Kind::ASSIGN_SUB:
      case Node::Kind::ASSIGN_MUL:
      case Node::Kind::ASSIGN_DIV:
      case Node::Kind::ASSIGN_MOD:
      case Node::Kind::ASSIGN_BIT_AND:
      case Node::Kind::ASSIGN_BIT_OR:
      case Node::Kind::ASSIGN_BIT_XOR:
      case Node::Kind::ASSIGN_RSHIFT:
      case Node::Kind::ASSIGN_LSHIFT:
      case Node::Kind::LOGICAL_AND:
      case Node::Kind::LOGICAL_OR:
      case Node::Kind::TUPLE_TYPE:
      case Node::Kind::UNION_TYPE: {
        auto op = static_cast<const Oper*>(node);
        out << "(#" << Node::KindName(op->kind);
        visitList(op->operands);
        out << ')';
        break;
      }

      // case Node::Kind::RANGE: return "RANGE";
      // case Node::Kind::AS_TYPE: return "AS_TYPE";
      // case Node::Kind::IS: return "IS";
      // case Node::Kind::IS_NOT: return "IS_NOT";
      // case Node::Kind::IN: return "IN";
      // case Node::Kind::NOT_IN: return "NOT_IN";
      // case Node::Kind::RETURNS: return "RETURNS";
      // case Node::Kind::LAMBDA: return "LAMBDA";
      // case Node::Kind::EXPR_TYPE: return "EXPR_TYPE";
      // case Node::Kind::RETURN: return "RETURN";
      // case Node::Kind::THROW: return "THROW";


      // case Node::Kind::SPECIALIZE: return "SPECIALIZE";
      // case Node::Kind::CALL: return "CALL";
      // case Node::Kind::FLUENT_MEMBER: return "FLUENT_MEMBER";
      // case Node::Kind::ARRAY_LITERAL: return "ARRAY_LITERAL";
      // case Node::Kind::LIST_LITERAL: return "LIST_LITERAL";
      // case Node::Kind::SET_LITERAL: return "SET_LITERAL";
      // case Node::Kind::CALL_REQUIRED: return "CALL_REQUIRED";
      // case Node::Kind::CALL_REQUIRED_STATIC: return "CALL_REQUIRED_STATIC";
      // case Node::Kind::LIST: return "LIST";

      case Node::Kind::BLOCK: {
        auto blk = static_cast<const Block*>(node);
        out << "(#" << Node::KindName(blk->kind);
        visitList(blk->stmts);
        visitNamedNode(blk->result, "result");
        out << ')';
        break;
      }

      // case Node::Kind::BLOCK: return "BLOCK";
      // case Node::Kind::CONST: return "CONST";
      // case Node::Kind::VAR_DEFN: return "VAR_DEFN";
      // case Node::Kind::STATIC: return "STATIC";
      // case Node::Kind::ELSE: return "ELSE";
      // case Node::Kind::FINALLY: return "FINALLY";
      case Node::Kind::IF: {
        auto stmt = static_cast<const Control*>(node);
        out << "(#" << Node::KindName(stmt->kind);
        visitList(stmt->test);
        visitNamedList(stmt->outcomes, "outcomes");
        out << ')';
        break;
      }
      // case Node::Kind::WHILE: return "WHILE";
      // case Node::Kind::LOOP: return "LOOP";
      // case Node::Kind::FOR: return "FOR";
      // case Node::Kind::FOR_IN: return "FOR_IN";
      // case Node::Kind::TRY: return "TRY";
      // case Node::Kind::CATCH: return "CATCH";
      // case Node::Kind::SWITCH: return "SWITCH";
      // case Node::Kind::CASE: return "CASE";
      // case Node::Kind::MATCH: return "MATCH";
      // case Node::Kind::PATTERN: return "PATTERN";
      // case Node::Kind::MODIFIED: return "MODIFIED";
      // case Node::Kind::FUNCTION_TYPE: return "FUNCTION_TYPE";
      // case Node::Kind::BREAK: return "BREAK";
      // case Node::Kind::CONTINUE: return "CONTINUE";
      // case Node::Kind::VISIBILITY: return "VISIBILITY";
      // case Node::Kind::DEFN: return "DEFN";
      // case Node::Kind::TYPE_DEFN: return "TYPE_DEFN";
      // case Node::Kind::EXTEND_DEFN: return "EXTEND_DEFN";
      // case Node::Kind::OBJECT_DEFN: return "OBJECT_DEFN";
      // case Node::Kind::ENUM_DEFN: return "ENUM_DEFN";
      // case Node::Kind::VAR: return "VAR";
      // case Node::Kind::LET: return "LET";
      // case Node::Kind::VAR_LIST: return "VAR_LIST";
      // case Node::Kind::ENUM_VALUE: return "ENUM_VALUE";
      // case Node::Kind::PARAMETER: return "PARAMETER";
      // case Node::Kind::TYPE_PARAMETER: return "TYPE_PARAMETER";
      // case Node::Kind::FUNCTION: return "FUNCTION";
      // case Node::Kind::DEFN_END: return "DEFN_END";

      case Node::Kind::CLASS_DEFN:
      case Node::Kind::STRUCT_DEFN:
      case Node::Kind::INTERFACE_DEFN:
      case Node::Kind::TRAIT_DEFN:
      case Node::Kind::EXTEND_DEFN: {
        auto de = static_cast<const Defn*>(node);
        out << "(#" << Node::KindName(de->kind);
        visitDefnFlags(de);
        out << ' ' << de->name;
        visitNamedList(de->attributes, "attributes");
        visitNamedList(de->typeParams, "typeParams");
        visitNamedList(de->requires, "requires");
        visitList(de->members);
        out << ')';
        break;
      }

      case Node::Kind::ALIAS_DEFN: {
        auto de = static_cast<const TypeDefn*>(node);
        out << "(#" << Node::KindName(de->kind);
        visitDefnFlags(de);
        out << ' ' << de->name;
        visitNamedList(de->attributes, "attributes");
        visitNamedList(de->typeParams, "typeParams");
        visitList(de->extends);
        out << ')';
        break;
      }

      case Node::Kind::MEMBER_VAR:
      case Node::Kind::MEMBER_CONST: {
        auto de = static_cast<const ValueDefn*>(node);
        out << "(#" << Node::KindName(de->kind);
        visitDefnFlags(de);
        out << ' ' << de->name;
        if (de->type) {
          out << ": ";
          visit(de->type);
        }
        if (de->init) {
          out << " = ";
          visit(de->init);
        }
        out << ')';
        break;
      }

      case Node::Kind::TYPE_PARAMETER: {
        auto tp = static_cast<const TypeParameter*>(node);
        out << "(#" << Node::KindName(tp->kind);
        visitDefnFlags(tp);
        out << ' ' << tp->name;
        // if (tp->type) {
        //   out << ": ";
        //   visit(tp->type);
        // }
        if (tp->init) {
          out << " = ";
          visit(tp->init);
        }
        out << ')';
        break;
      }

      case Node::Kind::FUNCTION: {
        auto de = static_cast<const Function*>(node);
        out << "(#" << Node::KindName(de->kind);
        visitDefnFlags(de);
        out << ' ' << de->name;
        visitNamedList(de->attributes, "params");
        if (de->returnType) {
          out << ": ";
          visit(de->returnType);
        }
        if (de->body) {
          out << " ";
          visit(de->body);
        }
        out << ')';
        break;
      }

      case Node::Kind::MODULE: {
        auto mod = static_cast<const Module*>(node);
        out << "(#MODULE";
        visitList(mod->imports);
        visitList(mod->members);
        out << ')';
        break;
      }

      case Node::Kind::IMPORT:
      case Node::Kind::EXPORT: {
        auto imp = static_cast<const Import*>(node);
        out << "(#" << Node::KindName(imp->kind) << " " << imp->relative << " ";
        out << '"' << imp->path << '"';
        visitList(imp->members);
        out << ')';
        break;
      }

      default:
        diag.error() << "Format not implemented: " << Node::KindName(node->kind);
        assert(false && "Format not implemented.");
    }
  }

  void Formatter::visitList(const NodeList& nodes) {
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

  void Formatter::visitNamedList(const NodeList& nodes, llvm::StringRef name) {
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

  void Formatter::visitNamedNode(const Node* node, llvm::StringRef name) {
    if (node) {
      if (pretty) {
        indent += 2;
        out << '\n' << std::string(indent, ' ') << name << ": ";
        visit(node);
        // out << ')';
        indent -= 2;
      } else {
        out << "(" << name;
        visit(node);
        out << ')';
      }
    }
  }

  void Formatter::visitDefnFlags(const Defn* de) {
    printFlag("private", de->isPrivate());
    printFlag("protected", de->isProtected());
    printFlag("static", de->isStatic());
    printFlag("final", de->isFinal());
    printFlag("abstract", de->isAbstract());
  }

  void Formatter::printFlag(llvm::StringRef name, bool enabled) {
    if (enabled) {
      if (pretty) {
        out << '\n' << std::string(indent + 2, ' ') << "#" << name;
      } else {
        out << " #" << name;
      }
    }
  }


  void format(::std::ostream& out, const Node* node, bool pretty) {
    Formatter fmt(out, pretty);
    fmt.visit(node);
    if (pretty) {
      out << '\n';
    }
  }
}
