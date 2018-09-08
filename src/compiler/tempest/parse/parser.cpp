#include "tempest/ast/defn.hpp"
#include "tempest/ast/node.hpp"
#include "tempest/ast/ident.hpp"
#include "tempest/ast/literal.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/ast/oper.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/parse/nodelistbuilder.hpp"
#include "llvm/ADT/SmallString.h"

#include <cassert>

namespace tempest::parse {
  using tempest::ast::Defn;
  using tempest::ast::Node;
  using tempest::error::diag;
  using llvm::StringRef;

  enum precedence {
    PREC_COMMA = 0,
    PREC_FAT_ARROW,
    PREC_RETURNS,
    PREC_ASSIGN,
    PREC_RANGE,   // And colon
    PREC_LOGICAL_AND, // 'and' and 'or'
    PREC_LOGICAL_OR, // 'and' and 'or'
    PREC_NOT,     // 'not'
    PREC_IN,      // 'in'
    PREC_IS_AS,   // 'is' and 'as'
    PREC_RELATIONAL,  // comparison
    PREC_BIT_OR,  // bitwise-or
    PREC_BIT_XOR, // bitwise-xor
    PREC_BIT_AND, // bitwise-and
    PREC_SHIFT,   // shift operators
    PREC_ADD_SUB, // binary plus and minus
    PREC_MUL_DIV, // multiply and divide
  };

  Parser::Parser(ProgramSource* source, tempest::support::BumpPtrAllocator& alloc)
    : _alloc(alloc)
    , _lexer(source)
    , _recovering(false)
  {
    _token = _lexer.next();
  }

  void Parser::next() {
    _prevToken = _token;
    _token = _lexer.next();
  }

  bool Parser::match(TokenType tok) {
    if (_token == tok) {
      next();
      return true;
    }
    return false;
  }

  // Module

  ast::Module* Parser::module() {
    NodeListBuilder imports(_alloc);
    NodeListBuilder members(_alloc);

    ast::Module* mod = new (_alloc) ast::Module(location());
    bool declDefined = false;
    while (_token != TOKEN_END) {
      if (match(TOKEN_IMPORT)) {
        if (declDefined) {
          diag.error(location()) << "Import statement must come before the first declaration.";
        }
        auto imp = importStmt(ast::Node::Kind::IMPORT);
        if (imp) {
          imports.append(imp);
        } else {
          skipOverDefn();
        }
      } else if (match(TOKEN_EXPORT)) {
        if (_token == TOKEN_LBRACE) {
          if (declDefined) {
            diag.error(location()) << "Export statement must come before the first declaration.";
          }
          auto imp = importStmt(ast::Node::Kind::EXPORT);
          if (imp) {
            imports.append(imp);
          } else {
            skipOverDefn();
          }
        } else {
          Defn* d = declaration(DECL_GLOBAL);
          if (d) {
            members.append(d);
            declDefined = true;
          }
        }
      } else {
        Defn* d = declaration(DECL_GLOBAL);
        if (d) {
          members.append(d);
          declDefined = true;
        }
      }
    }

    mod->imports = imports.build();
    mod->members = members.build();
    return mod;
  }

  // Import

  Node* Parser::importStmt(ast::Node::Kind kind) {
    auto loc = location();
    if (!match(TOKEN_LBRACE)) {
      expected("{");
      return nullptr;
    }

    NodeListBuilder members(_alloc);
    for (;;) {
      if (_token != TOKEN_ID) {
        diag.error(location()) << "Identifier expected.";
        return nullptr;
      }
      auto importSym = id();
      if (match(TOKEN_AS)) {
        if (_token != TOKEN_ID) {
          diag.error(location()) << "Identifier expected.";
          return nullptr;
        }
        // KeywordArg is a handy node type to store the alias for now.
        importSym = new (_alloc) ast::KeywordArg(
          importSym->location, copyOf(tokenValue()), importSym);
      }
      members.append(importSym);
      if (match(TOKEN_RBRACE)) {
        break;
      } else if (!match(TOKEN_COMMA)) {
        expected("{");
        return nullptr;
      }
    }

    if (!match(TOKEN_FROM)) {
      expected("from");
      return nullptr;
    }

    int32_t relativePath = 0;
    while (match(TOKEN_DOT)) {
      relativePath += 1;
    }

    if (_token != TOKEN_ID) {
      expected("module name");
      return nullptr;
    }
    auto path = dottedIdentStr();

    if (!match(TOKEN_SEMI)) {
      diag.error(location()) << "Semicolon expected.";
      return nullptr;
    }

    auto result = new (_alloc) ast::Import(kind, loc, path, relativePath);
    result->members = members.build();
    return result;
  }

  // Declaration

  bool Parser::memberDeclaration(NodeListBuilder& decls) {
    NodeListBuilder attributes(_alloc);
    while (auto attr = attribute()) {
      attributes.append(attr);
    }

    Defn* d;
    bool isPrivate = false;
    bool isProtected = false;
    if (_token == TOKEN_PRIVATE || _token == TOKEN_PROTECTED) {
      isPrivate = match(TOKEN_PRIVATE);
      isProtected = match(TOKEN_PROTECTED);
      if (match(TOKEN_LBRACE)) {
        // It's a block of definitions with common visibility.
        if (attributes.size() > 0) {
          diag.error(location()) << "Visibility block may not have attributes.";
        }
        while (!match(TOKEN_RBRACE)) {
          if (_token == TOKEN_END) {
            diag.error(location()) << "Visibility block not closed.";
            return false;
          }
          NodeListBuilder attributes2(_alloc);
          while (auto attr = attribute()) {
            attributes2.append(attr);
          }
          d = declaration(DECL_MEMBER);
          if (d) {
            d->attributes = attributes2.build();
            d->setPrivate(isPrivate);
            d->setProtected(isProtected);
            decls.append(d);
          } else {
            return false;
          }
        }
        return true;
      }
    }

    d = declaration(DECL_MEMBER);
    if (d != nullptr) {
      d->attributes = attributes.build();
      d->setPrivate(isPrivate);
      d->setProtected(isProtected);
      decls.append(d);
      return true;
    }
    diag.error(location()) << "declaration expected.";
    return false;
  }

  Defn* Parser::declaration(DeclarationScope scope) {
    NodeListBuilder attributes(_alloc);
    while (auto attr = attribute()) {
      attributes.append(attr);
    }

    Defn* result = nullptr;
    bool isAbstract = false;
    bool isFinal = false;
    bool isStatic = false;
    for (;;) {
      if (match(TOKEN_ABSTRACT)) {
        if (isAbstract) {
          diag.error(location()) << "'abstract' already specified.";
        }
        isAbstract = true;
      } else if (match(TOKEN_FINAL)) {
        if (isFinal) {
          diag.error(location()) << "'final' already specified.";
        }
        isFinal = true;
      } else if (match(TOKEN_STATIC)) {
        if (isStatic) {
          diag.error(location()) << "'static' already specified.";
        }
        isStatic = true;
      } else {
        break;
      }
    }

    switch (_token) {
      case TOKEN_CONST:
        result = fieldDef();
        break;
      case TOKEN_LET:
        if (scope == DECL_MEMBER) {
          diag.error(location()) << "'let' keyword not needed for member declaration.";
        } else {
          result = fieldDef();
        }
        break;
      case TOKEN_ID:
        if (scope == DECL_GLOBAL) {
          diag.error(location()) << "Declaration expected.";
        } else {
          result = fieldDef();
        }
        break;
      case TOKEN_CLASS:
      case TOKEN_STRUCT:
      case TOKEN_INTERFACE:
      case TOKEN_EXTEND:
      case TOKEN_OBJECT:
      case TOKEN_TRAIT:
        result = compositeTypeDef();
        break;
      case TOKEN_ENUM:
        result = enumTypeDef();
        break;
      case TOKEN_TYPE:
        result = aliasTypeDef();
        break;
      case TOKEN_FN:
      case TOKEN_UNDEF:
      case TOKEN_OVERRIDE:
      case TOKEN_GET:
      case TOKEN_SET:
        result = methodDef();
        break;
      default:
        diag.error(location()) << "Declaration expected.";
        skipOverDefn();
        return nullptr;
    }

    result->setAbstract(isAbstract);
    result->setFinal(isFinal);
    result->setStatic(isStatic);
    result->attributes = attributes.build();
    return result;
  }

  // Composite types

  Defn* Parser::compositeTypeDef() {
    Node::Kind kind;
    switch (_token) {
      case TOKEN_CLASS:       kind = Node::Kind::CLASS_DEFN; break;
      case TOKEN_STRUCT:      kind = Node::Kind::STRUCT_DEFN; break;
      case TOKEN_INTERFACE:   kind = Node::Kind::INTERFACE_DEFN; break;
      case TOKEN_EXTEND:      kind = Node::Kind::EXTEND_DEFN; break;
      case TOKEN_OBJECT:      kind = Node::Kind::OBJECT_DEFN; break;
      case TOKEN_TRAIT:       kind = Node::Kind::TRAIT_DEFN; break;
      default:
        assert(false);
        break;
    }
    next();

    if (_token != TOKEN_ID) {
      diag.error(location()) << "Type name expected.";
      _recovering = true;
    }
    StringRef name = copyOf(tokenValue());
    Location loc = location();
    next();

    ast::TypeDefn* d = new (_alloc) ast::TypeDefn(kind, loc, name);

    // Template parameters
    NodeListBuilder templateParams(_alloc);
    templateParamList(templateParams);
    d->typeParams = templateParams.build();

    // Supertype list
    if (match(TOKEN_EXTENDS)) {
      NodeListBuilder extends(_alloc);
      for (;;) {
        auto base = baseTypeName();
        if (base == nullptr) {
          skipUntil({TOKEN_LBRACE});
          return d;
        }
        extends.append(base);
        if (!match(TOKEN_COMMA)) {
          break;
        }
      }
      d->extends = extends.build();
    }

    if (match(TOKEN_IMPLEMENTS)) {
      NodeListBuilder implements(_alloc);
      for (;;) {
        auto base = baseTypeName();
        if (base == nullptr) {
          skipUntil({TOKEN_LBRACE});
          return d;
        }
        implements.append(base);
        if (!match(TOKEN_COMMA)) {
          break;
        }
      }
      d->implements = implements.build();
    }

    // Type constraints
    NodeListBuilder requires(_alloc);
    requirements(requires);
    d->requires = requires.build();

    // Body
    if (!match(TOKEN_LBRACE)) {
      expected("{");
      skipOverDefn();
    } else {
      if (!classBody(d)) {
        return nullptr;
      }
    }

    return d;
  }

  bool Parser::classBody(ast::TypeDefn* d) {
    NodeListBuilder members(_alloc);
    NodeListBuilder friends(_alloc);
    while (_token != TOKEN_END && !match(TOKEN_RBRACE)) {
      if (!classMember(members, friends)) {
        return false;
      }
    }
    d->members = members.build();
    d->friends = friends.build();
    return true;
  }

  bool Parser::classMember(NodeListBuilder &members, NodeListBuilder &friends) {
    if (match(TOKEN_FRIEND)) {
      auto friendDecl = dottedIdent();
      if (friendDecl == nullptr) {
        expected("class or function name");
        skipUntil({TOKEN_SEMI, TOKEN_RBRACE});
      }
      if (!match(TOKEN_SEMI)) {
        expected("';'");
      }
      friends.append(friendDecl);
    } else if (!memberDeclaration(members)) {
      return false;
    }
    return true;
  }

  // Enumeration types

  // Definitions allowed inside of an enumeration.
  static const std::unordered_set<TokenType> ENUM_MEMBERS = {
    TOKEN_FN, TOKEN_OVERRIDE, TOKEN_UNDEF, TOKEN_LET,
    TOKEN_CLASS, TOKEN_STRUCT, TOKEN_INTERFACE, TOKEN_EXTEND, TOKEN_ENUM
  };

  Defn* Parser::enumTypeDef() {
    next();
    if (_token != TOKEN_ID) {
      diag.error(location()) << "Type name expected.";
      _recovering = true;
    }
    StringRef name = copyOf(tokenValue());
    Location loc = location();
    next();

    ast::TypeDefn* d = new (_alloc) ast::TypeDefn(Node::Kind::ENUM_DEFN, loc, name);

    // Supertype list
    if (match(TOKEN_EXTENDS)) {
      NodeListBuilder extends(_alloc);
      for (;;) {
        auto base = enumBaseTypeName();
        if (base == nullptr) {
          skipUntil({TOKEN_LBRACE});
          return d;
        }
        extends.append(base);
        if (!match(TOKEN_COMMA)) {
          break;
        }
      }
      d->extends = extends.build();
    }

    // Body
    if (!match(TOKEN_LBRACE)) {
      expected("{");
      skipOverDefn();
    } else {
      NodeListBuilder members(_alloc);
      NodeListBuilder friends(_alloc);
      bool defnMode = false;
      while (!match(TOKEN_RBRACE)) {
        if (_token == TOKEN_END) {
          diag.error(loc) << "Incomplete enum.";
          return nullptr;
        }
        if (defnMode || ENUM_MEMBERS.find(_token) != ENUM_MEMBERS.end()) {
          defnMode = true;
          if (!classMember(members, friends)) {
            return nullptr;
          }
        } else {
          if (!enumMember(members)) {
            return nullptr;
          }
        }

        if (match(TOKEN_COMMA)) {
          continue;
        } else if (match(TOKEN_SEMI)) {
          defnMode = true;
        }
      }

      d->members = members.build();
    }

    return d;
  }

  bool Parser::enumMember(NodeListBuilder &members) {
    if (_token != TOKEN_ID) {
      diag.error(location()) << "Identifier expected.";
      _recovering = true;
    }

    StringRef name = copyOf(tokenValue());
    Location loc = location();
    next();

    auto ev = new (_alloc) ast::EnumValue(loc, name);

    // Initializer
    if (match(TOKEN_ASSIGN)) {
      auto init = expression();
      if (init != nullptr) {
        ev->init = init;
      }
    } else if (match(TOKEN_LPAREN)) {
      NodeListBuilder args(_alloc);
      Location callLoc = loc;
      if (!callingArgs(args, callLoc)) {
        return false;
      }
      ev->init = new (_alloc) ast::Oper(Node::Kind::CALL, callLoc, args.build());
    }
    members.append(ev);
    return true;
  }

  Defn* Parser::aliasTypeDef() {
    next();
    if (_token != TOKEN_ID) {
      diag.error(location()) << "Type name expected.";
      _recovering = true;
    }
    StringRef name = copyOf(tokenValue());
    Location loc = location();
    next();

    auto d = new (_alloc) ast::TypeDefn(Node::Kind::ALIAS_DEFN, loc, name);

    if (!match(TOKEN_ASSIGN)) {
      expected("=");
      skipOverDefn();
    }

    auto init = typeExpression();
    if (init == nullptr) {
      skipOverDefn();
    } else {
      // For alias we'll consider the target an 'extends'.
      NodeListBuilder extends(_alloc);
      extends.append(init);
      d->extends = extends.build();
    }

    if (!match(TOKEN_SEMI)) {
      diag.error(location()) << "Semicolon expected.";
      skipUntil({TOKEN_SEMI});
      next();
    }

    return d;
  }

  // Method

  Defn* Parser::methodDef() {
    bool isUndef = false;
    bool isOverride = false;
    bool isGetter = false;
    bool isSetter = false;
    if (match(TOKEN_UNDEF)) {
      isUndef = true;
    } else if (match(TOKEN_OVERRIDE)) {
      isOverride = true;
    }

    if (match(TOKEN_GET)) {
      isGetter = true;
    } else if (match(TOKEN_SET)) {
      isSetter = true;
    }

    if (!isUndef && !isOverride && !isGetter && !isSetter) {
      if (!match(TOKEN_FN)) {
        assert(false && "Invalid function token.");
      }
    }

    // Method name (may be empty).
    Location loc = location();
    StringRef name;
    if (_token == TOKEN_ID) {
      name = copyOf(tokenValue());
      next();
    }

    // Template parameters
    NodeListBuilder templateParams(_alloc);
    templateParamList(templateParams);

    NodeListBuilder params(_alloc);
    if (_token == TOKEN_LPAREN) {
      if (!paramList(params)) {
        skipOverDefn();
        return nullptr;
      }
    } else if (name.empty() && !isGetter) {
      expected("function parameter list");
      skipOverDefn();
      return nullptr;
    }

    if (name.empty()) {
      name = "()";
    }

    Node* returnType = nullptr;
    if (match(TOKEN_RETURNS)) {
      returnType = typeExpression();
      if (returnType == nullptr) {
        skipOverDefn();
        return nullptr;
      }
    }

    if (returnType != nullptr ||
        _token == TOKEN_WHERE ||
        _token == TOKEN_LBRACE ||
        _token == TOKEN_FAT_ARROW ||
        _token == TOKEN_SEMI) {
      // Method
      ast::Function* fn = new (_alloc) ast::Function(loc, name);
      fn->typeParams = templateParams.build();
      fn->params = params.build();
      fn->setOverride(isOverride);
      fn->setUndef(isUndef);
      fn->getter = isGetter;
      fn->setter = isSetter;
      fn->returnType = returnType;
      for (auto p : fn->params) {
        if (static_cast<const ast::Parameter*>(p)->variadic) {
          fn->variadic = true;
        } else if (fn->variadic) {
          diag.error(fn) << "Only the last parameter in a function can be variadic.";
        }
      }

      // Type constraints
      NodeListBuilder requires(_alloc);
      requirements(requires);
      fn->requires = requires.build();

      auto body = methodBody();
      if (body == &Node::ERROR) {
        skipOverDefn();
        return nullptr;
      } else if (body != nullptr && body->kind != Node::Kind::ABSENT) {
        fn->body = body;
      }
      return fn;
    } else {
      diag.error(location()) << "return type or function body expected.";
      skipOverDefn();
      return nullptr;
    }
  }

  Node* Parser::methodBody() {
    if (match(TOKEN_SEMI)) {
      return &Node::ABSENT;
    } else if (_token == TOKEN_LBRACE) {
      auto body = block();
      if (body != nullptr) {
        return body;
      }
    } else if (match(TOKEN_FAT_ARROW)) {
      auto body = exprList();
      if (body != nullptr) {
        if (!match(TOKEN_SEMI)) {
          expected("';'");
        }
        return body;
      }
    } else {
      diag.error(location()) << "Method body expected.";
    }
    return &Node::ERROR;
  }

  // Requirements

  bool Parser::requirements(NodeListBuilder& out) {
    if (match(TOKEN_WHERE)) {
      for (;;) {
        auto req = requirement();
        if (req == nullptr) {
          skipUntil({TOKEN_LBRACE});
          return false;
        }
        out.append(req);
        if (!match(TOKEN_AND)) {
          break;
        }
      }
    }
    return true;
  }

  Node* Parser::requirement() {
    Location loc = location();
    if (match(TOKEN_INC)) {
      auto operand = typeExpression();
      if (operand == nullptr) {
        return nullptr;
      }
      return new (_alloc) ast::UnaryOp(Node::Kind::PRE_INC, loc, operand);
    } else if (match(TOKEN_INC)) {
      auto operand = typeExpression();
      if (operand == nullptr) {
        return nullptr;
      }
      return new (_alloc) ast::UnaryOp(Node::Kind::PRE_DEC, loc, operand);
    } else if (match(TOKEN_MINUS)) {
      auto operand = typeExpression();
      if (operand == nullptr) {
        return nullptr;
      }
      return new (_alloc) ast::UnaryOp(Node::Kind::NEGATE, loc, operand);
    } else if (match(TOKEN_STATIC)) {
      auto fn = typeExpression();
      if (fn == nullptr) {
        return nullptr;
      }
      return requireCall(Node::Kind::CALL_REQUIRED_STATIC, fn);
    }

    auto rqTerm = typeExpression();
    if (rqTerm == nullptr) {
      return nullptr;
    }
    switch (_token) {
      case TOKEN_PLUS:    return requireBinaryOp(Node::Kind::ADD, rqTerm);
      case TOKEN_MINUS:   return requireBinaryOp(Node::Kind::SUB, rqTerm);
      case TOKEN_MUL:     return requireBinaryOp(Node::Kind::MUL, rqTerm);
      case TOKEN_DIV:     return requireBinaryOp(Node::Kind::DIV, rqTerm);
      case TOKEN_MOD:     return requireBinaryOp(Node::Kind::MOD, rqTerm);
      case TOKEN_VBAR:    return requireBinaryOp(Node::Kind::BIT_OR, rqTerm);
      case TOKEN_CARET:   return requireBinaryOp(Node::Kind::BIT_XOR, rqTerm);
      case TOKEN_AMP:     return requireBinaryOp(Node::Kind::BIT_AND, rqTerm);
      case TOKEN_RSHIFT:  return requireBinaryOp(Node::Kind::RSHIFT, rqTerm);
      case TOKEN_LSHIFT:  return requireBinaryOp(Node::Kind::LSHIFT, rqTerm);
      case TOKEN_EQ:      return requireBinaryOp(Node::Kind::EQUAL, rqTerm);
      case TOKEN_NE:      return requireBinaryOp(Node::Kind::NOT_EQUAL, rqTerm);
      case TOKEN_LT:      return requireBinaryOp(Node::Kind::LESS_THAN, rqTerm);
      case TOKEN_GT:      return requireBinaryOp(Node::Kind::GREATER_THAN, rqTerm);
      case TOKEN_LE:      return requireBinaryOp(Node::Kind::LESS_THAN_OR_EQUAL, rqTerm);
      case TOKEN_GE:      return requireBinaryOp(Node::Kind::GREATER_THAN_OR_EQUAL, rqTerm);
      case TOKEN_TYPE_LE: return requireBinaryOp(Node::Kind::IS_SUB_TYPE, rqTerm);
      case TOKEN_TYPE_GE: return requireBinaryOp(Node::Kind::IS_SUPER_TYPE, rqTerm);
      case TOKEN_IN:      return requireBinaryOp(Node::Kind::IN, rqTerm);
      case TOKEN_RANGE:   return requireBinaryOp(Node::Kind::RANGE, rqTerm);

      case TOKEN_INC:
        next();
        return new (_alloc) ast::UnaryOp(Node::Kind::POST_INC, rqTerm->location, rqTerm);
      case TOKEN_DEC:
        next();
        return new (_alloc) ast::UnaryOp(Node::Kind::POST_DEC, rqTerm->location, rqTerm);

      case TOKEN_NOT: {
        next();
        if (!match(TOKEN_IN)) {
          diag.error(location()) <<
              "'not' must be followed by 'in' when used as a binary operator.";
          return nullptr;
        }
        return requireBinaryOp(Node::Kind::NOT_IN, rqTerm);
      }

      case TOKEN_LPAREN: {
        return requireCall(Node::Kind::CALL_REQUIRED, rqTerm);
      }

      default:
        return rqTerm;
    }
    assert(false);
  }

  Node* Parser::requireBinaryOp(Node::Kind kind, Node* left) {
    next();
    auto right = typeExpression();
    if (right == nullptr) {
      return nullptr;
    }
    NodeListBuilder builder(_alloc);
    builder.append(left);
    builder.append(right);
    return new (_alloc) ast::Oper(
      kind, left->location | right ->location, builder.build());
  }

  Node* Parser::requireCall(Node::Kind kind, Node* fn) {
    auto fnType = functionType();
    if (fnType == nullptr) {
      return nullptr;
    }
    NodeListBuilder signature(_alloc);
    signature.append(fnType);
    ast::Oper *call = new (_alloc) ast::Oper(kind, fn->location, signature.build());
    call->op = fn;
    return call;
  }

  // Method and Property parameter lists

  bool Parser::paramList(NodeListBuilder& builder) {
    bool keywordOnly = false;
    if (match(TOKEN_LPAREN)) {
      if (match(TOKEN_RPAREN)) {
        return true;
      }

      if (match(TOKEN_SEMI)) {
        keywordOnly = true;
      }

      for (;;) {
        // Parameter name
        ast::Parameter* param = nullptr;
        if (_token == TOKEN_ID) {
          param = new (_alloc) ast::Parameter(location(), copyOf(tokenValue()));
          next();
        } else if (match(TOKEN_SELF)) {
          param = new (_alloc) ast::Parameter(location(), copyOf(tokenValue()));
          param->selfParam = true;
        } else if (match(TOKEN_CLASS)) {
          param = new (_alloc) ast::Parameter(location(), copyOf(tokenValue()));
          param->classParam = true;
        } else {
          expected("parameter name");
          skipUntil({TOKEN_COMMA, TOKEN_RBRACE});
          if (match(TOKEN_COMMA)) {
            continue;
          } else if (match(TOKEN_RBRACE)) {
            break;
          } else {
            return false;
          }
        }

        if (param) {
          // Parameter type
          param->keywordOnly = keywordOnly;
          if (match(TOKEN_COLON)) {
            if (match(TOKEN_REF)) {
              diag.debug(location()) << "here";
              assert(false && "Implement ref param");
            }
            if (match(TOKEN_ELLIPSIS)) {
              param->expansion = true;
            }
            Node* paramType;
            if (param->selfParam || param->classParam) {
              paramType = typeTerm(true);
            } else {
              paramType = typeExpression();
            }
            if (paramType == nullptr) {
              skipUntil({TOKEN_COMMA, TOKEN_RPAREN});
            } else {
              param->type = paramType;
            }

            if (match(TOKEN_ELLIPSIS)) {
              param->variadic = true;
            }
          }

          // Parameter initializer
          if (match(TOKEN_ASSIGN)) {
            auto init = expression();
            if (init != nullptr) {
              param->init = init;
            }
          }

          builder.append(param);

          if (match(TOKEN_RPAREN)) {
            break;
          }

          if (_token != TOKEN_SEMI && _token != TOKEN_COMMA) {
            expected("',' or '}'");
            skipUntil({TOKEN_COMMA, TOKEN_RPAREN});
          }

          if (match(TOKEN_SEMI)) {
            keywordOnly = true;
          } else {
            next();
          }
        }
      }
    }
    return true;
  }

  // Variable

  Defn* Parser::fieldDef() {
    Node::Kind kind;
    if (match(TOKEN_CONST)) {
      kind = Node::Kind::MEMBER_CONST;
    } else if (match(TOKEN_LET)) {
      kind = Node::Kind::MEMBER_VAR;
    } else if (_token == TOKEN_ID) {
      kind = Node::Kind::MEMBER_VAR;
    } else {
      assert(false);
    }

    ast::ValueDefn* var = varDecl(kind);

    // Initializer
    if (match(TOKEN_ASSIGN)) {
      auto init = exprList();
      if (init != nullptr) {
        var->init = init;
      }
    }

    if (!match(TOKEN_SEMI)) {
      diag.error(location()) << "Semicolon expected.";
      skipUntil({TOKEN_SEMI});
      next();
    }

    return var;
  };

  ast::ValueDefn* Parser::varDeclList(Node::Kind kind) {
    Location loc = location();
    ast::ValueDefn* var = varDecl(kind);
    if (var == nullptr) {
      return nullptr;
    }

    // Handle multiple variables, i.e. var x, y = ...
    // if (match(TOKEN_COMMA)) {
    //   NodeListBuilder varList(_alloc);
    //   varList.append(var);

    //   for (;;) {
    //     ast::ValueDefn* nextVar = varDecl(kind);
    //     if (nextVar == nullptr) {
    //       return nullptr;
    //     }
    //     varList.append(nextVar);
    //     if (!match(TOKEN_COMMA)) {
    //       break;
    //     }
    //   }

    //   var = new (_alloc) ast::ValueDefn(Node::Kind::VAR_LIST, loc, "");
    //   var->members = varList.build();
    // }

    return var;
  }

  ast::ValueDefn* Parser::varDecl(Node::Kind kind) {
    if (_token != TOKEN_ID) {
      diag.error(location()) << "Variable name expected.";
      _recovering = true;
    }
    StringRef name = copyOf(tokenValue());
    Location loc = location();
    next();

    ast::ValueDefn* val = new (_alloc) ast::ValueDefn(kind, loc, name);
    if (match(TOKEN_COLON)) {
      if (match(TOKEN_ELLIPSIS)) {
        assert(false && "Implement pre-ellipsis");
      }
      auto valType = typeExpression();
      if (valType == nullptr) {
        skipUntil({TOKEN_SEMI});
      } else {
        val->type = valType;
      }
    }

    return val;
  }

  // Declaration Modifiers

  Node* Parser::attribute() {
    Location loc = location();
    if (match(TOKEN_ATSIGN)) {
      if (_token != TOKEN_ID) {
        diag.error(location()) << "Attribute name expected.";
        skipUntil({
          TOKEN_ATSIGN, TOKEN_CLASS, TOKEN_STRUCT, TOKEN_INTERFACE, TOKEN_ENUM,
          TOKEN_FN, TOKEN_OVERRIDE, TOKEN_CONST, TOKEN_LET});
        return nullptr;
      }
      auto attr = dottedIdent();
      assert(attr != nullptr);
      if (match(TOKEN_LPAREN)) {
        NodeListBuilder args(_alloc);
        Location callLoc = loc;
        if (!callingArgs(args, callLoc)) {
          return nullptr;
        }
        ast::Oper* call = new (_alloc) ast::Oper(Node::Kind::CALL, callLoc, args.build());
        call->op = attr;
        attr = call;
      }
      return attr;
    } else if (match(TOKEN_INTRINSIC)) {
      return new (_alloc) ast::BuiltinAttribute(loc, ast::BuiltinAttribute::INTRINSIC);
    } else if (match(TOKEN_TRACEMETHOD)) {
      return new (_alloc) ast::BuiltinAttribute(loc, ast::BuiltinAttribute::TRACEMETHOD);
    } else if (match(TOKEN_UNSAFE)) {
      return new (_alloc) ast::BuiltinAttribute(loc, ast::BuiltinAttribute::UNSAFE);
    } else {
      return nullptr;
    }
  }

  // Template Params

  void Parser::templateParamList(NodeListBuilder& builder) {
    if (match(TOKEN_LBRACKET)) {
      Location loc = location();
      if (match(TOKEN_RBRACKET)) {
        diag.error(loc) << "Empty template parameter list.";
        return;
      }
      for (;;) {
        ast::TypeParameter* tp = templateParam();
        if (tp == nullptr) {
          skipUntil({TOKEN_RBRACKET});
          return;
        }
        builder.append(tp);
        if (match(TOKEN_RBRACKET)) {
          return;
        } else if (!match(TOKEN_COMMA)) {
          diag.error(loc) << "',' or ']' expected.";
        }
      }
    }
  }

  ast::TypeParameter* Parser::templateParam() {
    if (_token == TOKEN_ID) {
      ast::TypeParameter* tp = new (_alloc) ast::TypeParameter(location(), copyOf(tokenValue()));
      next();

      if (match(TOKEN_COLON)) {
        NodeListBuilder builder(_alloc);
        for (;;) {
          auto type = typeExpression();
          if (type == nullptr) {
            skipUntil({TOKEN_RBRACKET, TOKEN_LBRACE});
            break;
          } else {
            builder.append(type);
          }
          if (!match(TOKEN_AMP)) {
            break;
          }
        }
        tp->constraints = builder.build();
      } else {
        if (match(TOKEN_TYPE_LE)) {
          NodeListBuilder builder(_alloc);
          for (;;) {
            auto super = typeExpression();
            if (super == nullptr) {
              skipUntil({TOKEN_RBRACKET, TOKEN_LBRACE});
              break;
            } else {
              builder.append(new (_alloc)
                  ast::UnaryOp(Node::Kind::SUPERTYPE_CONSTRAINT, super->location, super));
            }
            if (!match(TOKEN_AMP)) {
              break;
            }
          }
          tp->constraints = builder.build();
          diag.error(location()) << "Supertype constraints are not supported yet.";
        } else if (match(TOKEN_ELLIPSIS)) {
          tp->variadic = true;
        }

        if (match(TOKEN_ASSIGN)) {
          auto init = typeExpression();
          if (init == nullptr) {
            skipUntil({TOKEN_RBRACKET, TOKEN_LBRACE});
          } else {
            tp->init = init;
          }
        }
      }
      return tp;
    } else {
      diag.error(location()) << "Type parameter name expected.";
      return nullptr;
    }
  }

  // Type Expressions

  Node* Parser::typeExpression() {
    return typeUnion();
  }

  Node* Parser::typeUnion() {
    auto t = typeTerm();
    if (match(TOKEN_VBAR)) {
      NodeListBuilder builder(_alloc);
      builder.append(t);
      while (_token != TOKEN_END) {
        t = typeTerm();
        if (t == nullptr) {
          return nullptr;
        }
        builder.append(t);
        if (!match(TOKEN_VBAR)) {
          break;
        }
      }
      return new ast::Oper(Node::Kind::UNION_TYPE, builder.location(), builder.build());
    }
    return t;
  }

  Node* Parser::typeTerm(bool allowPartial) {
    auto t = typePrimary(allowPartial);
    if (t && match(TOKEN_QMARK)) {
      return new (_alloc) ast::UnaryOp(Node::Kind::OPTIONAL, t->location, t);
    }
    return t;
  }

  Node* Parser::typePrimary(bool allowPartial) {
    switch (_token) {
      case TOKEN_CONST: {
        Location loc = location();
        next();
        Node::Kind kind = Node::Kind::CONST;
        if (match(TOKEN_QMARK)) {
          kind = Node::Kind::PROVISIONAL_CONST;
        }
        auto t = typePrimary(allowPartial);
        if (t == nullptr) {
          return nullptr;
        }
        return new (_alloc) ast::UnaryOp(kind, loc | t->location, t);
      }
      case TOKEN_FN: {
        next();
        return functionType();
      }
      case TOKEN_LBRACKET: {
        Location loc = location();
        next();
        auto t = typeExpression();
        if (t == nullptr) {
          return nullptr;
        }
        if (!match(TOKEN_RBRACKET)) {
          expected("']'");
        }
        return new (_alloc) ast::UnaryOp(Node::Kind::ARRAY_TYPE, loc | t->location, t);
      }
      case TOKEN_LPAREN: {
        Location loc = location();
        next();
        NodeListBuilder members(_alloc);
        bool trailingComma = true;
        if (!match(TOKEN_RPAREN)) {
          for (;;) {
            auto m = typeExpression();
            if (m == nullptr) {
              return nullptr;
            }
            members.append(m);
            loc |= location();
            if (match(TOKEN_RPAREN)) {
              trailingComma = false;
              break;
            } else if (!match(TOKEN_COMMA)) {
              expected("',' or ')'");
            } else if (match(TOKEN_RPAREN)) {
              trailingComma = true;
              break;
            }
          }
        }

        if (members.size() == 1 && !trailingComma) {
          return members[0];
        }
        return new (_alloc) ast::Oper(Node::Kind::TUPLE_TYPE, loc, members.build());
      }
      case TOKEN_ID: return specializedTypeName();
      case TOKEN_VOID: return builtinType(ast::BuiltinType::VOID);
      case TOKEN_BOOL: return builtinType(ast::BuiltinType::BOOL);
      case TOKEN_CHAR: return builtinType(ast::BuiltinType::CHAR);
      case TOKEN_I8: return builtinType(ast::BuiltinType::I8);
      case TOKEN_I16: return builtinType(ast::BuiltinType::I16);
      case TOKEN_I32: return builtinType(ast::BuiltinType::I32);
      case TOKEN_I64: return builtinType(ast::BuiltinType::I64);
      case TOKEN_INT: return builtinType(ast::BuiltinType::INT);
      case TOKEN_U8: return builtinType(ast::BuiltinType::U8);
      case TOKEN_U16: return builtinType(ast::BuiltinType::U16);
      case TOKEN_U32: return builtinType(ast::BuiltinType::U32);
      case TOKEN_U64: return builtinType(ast::BuiltinType::U64);
      case TOKEN_UINT: return builtinType(ast::BuiltinType::UINT);
      case TOKEN_FLOAT32: return builtinType(ast::BuiltinType::F32);
      case TOKEN_FLOAT64: return builtinType(ast::BuiltinType::F64);
      case TOKEN_STRING_LIT: return stringLit();
      case TOKEN_CHAR_LIT: return charLit();
      case TOKEN_DEC_INT_LIT:
      case TOKEN_HEX_INT_LIT: return integerLit();
      default: {
        if (allowPartial) {
          return &Node::ABSENT;
        }
        diag.error(location()) << "Type name expected.";
        return nullptr;
      }
    }
    return nullptr;
  }

  Node* Parser::functionType() {
    Location loc = location();
    NodeListBuilder params(_alloc);
    if (match(TOKEN_LPAREN)) {
      if (!match(TOKEN_RPAREN)) {
        for (;;) {
          auto arg = typeExpression();
          if (arg == nullptr) {
            return nullptr;
          }
          // TODO: Do we want to support ellipsis here?
          params.append(arg);
          loc |= location();
          if (match(TOKEN_RPAREN)) {
            break;
          } else if (!match(TOKEN_COMMA)) {
            expected("',' or ')'");
            return nullptr;
          }
        }
      }
    }

    ast::Oper *fnType = new (_alloc) ast::Oper(
        Node::Kind::FUNCTION_TYPE, loc, params.build());
    if (match(TOKEN_RETURNS)) {
      auto returnType = typeExpression();
      if (returnType == nullptr) {
        return nullptr;
      }
      loc |= returnType->location;
      fnType->op = returnType;
    }
    return fnType;
  }

  Node* Parser::baseTypeName() {
    if (_token == TOKEN_ID) {
      return specializedTypeName();
    } else {
      expected("base type name");
      return nullptr;
    }
  }

  Node* Parser::enumBaseTypeName() {
    switch (_token) {
      case TOKEN_ID: return specializedTypeName();
      case TOKEN_I8: return builtinType(ast::BuiltinType::I8);
      case TOKEN_I16: return builtinType(ast::BuiltinType::I16);
      case TOKEN_I32: return builtinType(ast::BuiltinType::I32);
      case TOKEN_I64: return builtinType(ast::BuiltinType::I64);
      case TOKEN_INT: return builtinType(ast::BuiltinType::INT);
      case TOKEN_U8: return builtinType(ast::BuiltinType::U8);
      case TOKEN_U16: return builtinType(ast::BuiltinType::U16);
      case TOKEN_U32: return builtinType(ast::BuiltinType::U32);
      case TOKEN_U64: return builtinType(ast::BuiltinType::U64);
      case TOKEN_UINT: return builtinType(ast::BuiltinType::UINT);
      default:
        expected("base type name");
        return nullptr;
    }
  }

  Node* Parser::specializedTypeName() {
    assert(_token == TOKEN_ID);
    auto type = id();
    assert(type != nullptr);
    for (;;) {
      Location loc = type->location;
      if (match(TOKEN_LBRACKET)) {
        NodeListBuilder builder(_alloc);
        loc = loc | location();
        if (!match(TOKEN_RBRACKET)) {
          for (;;) {
            auto typeArg = typeExpression();
            if (typeArg == nullptr) {
              return nullptr;
            }
            builder.append(typeArg);
            loc = loc | location();
            if (match(TOKEN_RBRACKET)) {
              break;
            } else if (!match(TOKEN_COMMA)) {
              expected("',' or ']'");
              return nullptr;
            }
          }
        }
        ast::Oper* spec = new (_alloc) ast::Oper(Node::Kind::SPECIALIZE, loc, builder.build());
        spec->op = type;
        type = spec;
      } else if (match(TOKEN_DOT)) {
        if (_token == TOKEN_ID) {
          type = new (_alloc) ast::MemberRef(location(), copyOf(tokenValue()), type);
          next();
        } else {
          expected("identifier");
          return nullptr;
        }
      } else {
        return type;
      }
    }
  }

  Node* Parser::builtinType(ast::BuiltinType::Type t) {
    auto result = new (_alloc) ast::BuiltinType(location(), t);
    next();
    return result;
  }

  // Statements

  Node* Parser::block() {
    Location loc = location();
    if (match(TOKEN_LBRACE)) {
      NodeListBuilder stmts(_alloc);
      Node* result = nullptr;
      if (!match(TOKEN_RBRACE)) {
        for (;;) {
          result = stmt();
          if (result == nullptr) {
            return nullptr; // Error, quit
          }
          // At this point the previous token should have been a right brace, or the
          // statement is waiting for a semicolon.
          TokenType stmtEnd = _prevToken;
          // If we see a semicolon, then 'result' is *NOT* the return result of the block.
          if (match(TOKEN_SEMI)) {
            stmts.append(result);
            result = nullptr;
            if (match(TOKEN_RBRACE)) {
              break;
            }
          } else if (match(TOKEN_RBRACE)) {
            // If we see the end of the block, then 'result' *IS* the result of the block and
            // we're done.
            break;
          } else if (stmtEnd != TOKEN_RBRACE) {
            // Otherwise if we saw neither a semicolon or a block and, and the previous statement
            // did not end in a right brace, then we're missing a semicolon.
            diag.error(location()) << "Semicolon expected after statement.";
          } else {
            stmts.append(result);
            result = nullptr;
          }
        }
      }
      // 'result' will be non-null if the last statement in the block was not followed by
      // a semicolon.
      return new (_alloc) ast::Block(loc, stmts.build(), result);
    }
    assert(false && "Missing opening brace.");
    return nullptr;
  }

  Node* Parser::requiredBlock() {
    if (_token != TOKEN_LBRACE) {
      diag.error(location()) << "Statement block required.";
      return nullptr;
    }
    return block();
  }

  Node* Parser::stmt() {
    switch (_token) {
      case TOKEN_IF:      return ifStmt();
      case TOKEN_WHILE:   return whileStmt();
      case TOKEN_LOOP:    return loopStmt();
      case TOKEN_FOR:     return forStmt();
      case TOKEN_SWITCH:  return switchStmt();
      case TOKEN_MATCH:   return matchStmt();
      case TOKEN_TRY:     return tryStmt();

      case TOKEN_BREAK: {
        auto st = new (_alloc) Node(Node::Kind::BREAK, location());
        next();
        return st;
      }

      case TOKEN_CONTINUE: {
        auto st = new (_alloc) Node(Node::Kind::CONTINUE, location());
        next();
        return st;
      }

      case TOKEN_RETURN:  return returnStmt();
      case TOKEN_THROW:   return throwStmt();

      case TOKEN_LET:
      case TOKEN_CONST:
        return localDefn();

      case TOKEN_FN:
      case TOKEN_CLASS:
      case TOKEN_STRUCT:
      case TOKEN_INTERFACE:
      case TOKEN_ENUM:
      case TOKEN_EXTEND:
        diag.error(location()) << "here";
        assert(false && "Implement local type");
        break;
      default: {
        return assignStmt();
      }
    }
  }

  Node* Parser::localDefn() {
    Node::Kind kind;
    if (match(TOKEN_CONST)) {
      kind = Node::Kind::LOCAL_CONST;
    } else {
      next();
      kind = Node::Kind::LOCAL_LET;
    }

    ast::ValueDefn* var = varDecl(kind);

    // Initializer
    if (match(TOKEN_ASSIGN)) {
      auto init = exprList();
      if (init != nullptr) {
        var->init = init;
      }
    }

    return var;
  }

  Node* Parser::assignStmt() {
    auto left = exprList();
    if (left == nullptr) {
      return nullptr;
    }

    Node::Kind kind = Node::Kind::ABSENT;
    switch (_token) {
      case TOKEN_ASSIGN: kind = Node::Kind::ASSIGN; break;
      case TOKEN_ASSIGN_PLUS: kind = Node::Kind::ASSIGN_ADD; break;
      case TOKEN_ASSIGN_MINUS: kind = Node::Kind::ASSIGN_SUB; break;
      case TOKEN_ASSIGN_MUL: kind = Node::Kind::ASSIGN_MUL; break;
      case TOKEN_ASSIGN_DIV: kind = Node::Kind::ASSIGN_DIV; break;
      case TOKEN_ASSIGN_MOD: kind = Node::Kind::ASSIGN_MOD; break;
      case TOKEN_ASSIGN_RSHIFT: kind = Node::Kind::ASSIGN_RSHIFT; break;
      case TOKEN_ASSIGN_LSHIFT: kind = Node::Kind::ASSIGN_LSHIFT; break;
      case TOKEN_ASSIGN_BITAND: kind = Node::Kind::ASSIGN_BIT_AND; break;
      case TOKEN_ASSIGN_BITOR: kind = Node::Kind::ASSIGN_BIT_OR; break;
      case TOKEN_ASSIGN_BITXOR: kind = Node::Kind::ASSIGN_BIT_XOR; break;
      default:
        return left;
    }

    next();
    auto right = exprList();
    if (right == nullptr) {
      return nullptr;
    }

    NodeListBuilder operands(_alloc);
    operands.append(left);
    operands.append(right);

    return new (_alloc) ast::Oper(kind, left->location | right->location, operands.build());
  }

  Node* Parser::ifStmt() {
    next();
    NodeListBuilder builder(_alloc);
    auto test = expression();
    if (test == nullptr) {
      return nullptr;
    }
    auto thenBlk = requiredBlock();
    if (thenBlk == nullptr) {
      return nullptr;
    }
    builder.append(thenBlk);
    if (match(TOKEN_ELSE)) {
      Node* elseBlk = nullptr;
      if (_token == TOKEN_IF) {
        elseBlk = ifStmt();
      } else {
        elseBlk = requiredBlock();
      }
      if (elseBlk != nullptr) {
        builder.append(elseBlk);
      }
    }

    return new (_alloc) ast::Control(Node::Kind::IF, test->location, test, builder.build());
  }

  Node* Parser::whileStmt() {
    next();
    NodeListBuilder builder(_alloc);
    auto test = expression();
    if (test == nullptr) {
      return nullptr;
    }
    auto body = requiredBlock();
    if (body == nullptr) {
      return nullptr;
    }
    builder.append(body);
    return new (_alloc) ast::Control(Node::Kind::WHILE, test->location, test, builder.build());
  }

  Node* Parser::loopStmt() {
    Location loc = location();
    next();
    NodeListBuilder builder(_alloc);
    auto body = requiredBlock();
    if (body == nullptr) {
      return nullptr;
    }
    builder.append(body);
    return new (_alloc) ast::Control(Node::Kind::LOOP, loc, nullptr, builder.build());
  }

  Node* Parser::forStmt() {
    Location loc = location();
    next();

    NodeListBuilder builder(_alloc);

    // if (_token == TOKEN_SEMI) {
    //   builder.append(&Node::ABSENT);
    // } else {
    ast::ValueDefn* var = varDeclList(Node::Kind::LOCAL_LET);

    builder.append(var);

    // It's a for-in statement
    if (!match(TOKEN_IN)) {
      expected("'in'");
      return nullptr;
    }

    // Iterator expression
    auto iter = expression();
    if (iter == nullptr) {
      return nullptr;
    }
    builder.append(iter);

    // Loop body
    auto body = requiredBlock();
    if (body == nullptr) {
      return nullptr;
    }
    builder.append(body);
    return new (_alloc) ast::Control(Node::Kind::FOR_IN, loc, nullptr, builder.build());
      // } else  if (match(TOKEN_ASSIGN)) {
      //   // Initializer
      //   auto init = exprList();
      //   if (init != nullptr) {
      //     var->init = init;
      //   }
      // }
    // }

    // if (!match(TOKEN_SEMI)) {
    //   expected("';'");
    //   return nullptr;
    // }

    // if (_token == TOKEN_SEMI) {
    //   builder.append(&Node::ABSENT);
    // } else {
    //   auto test = exprList();
    //   if (test == nullptr) {
    //     return nullptr;
    //   }
    //   builder.append(test);
    // }

    // if (!match(TOKEN_SEMI)) {
    //   expected("';'");
    //   return nullptr;
    // }

    // if (_token == TOKEN_SEMI) {
    //   builder.append(&Node::ABSENT);
    // } else {
    //   auto step = assignStmt();
    //   if (step == nullptr) {
    //     return nullptr;
    //   }
    //   builder.append(step);
    // }

    // auto body = requiredBlock();
    // if (body == nullptr) {
    //   return nullptr;
    // }

    // builder.append(body);
    // return new (_alloc) ast::Control(Node::Kind::FOR, loc, nullptr, builder.build());
  }

  Node* Parser::switchStmt() {
    Location loc = location();
    next();
    auto test = expression();
    if (test == nullptr) {
      return nullptr;
    }
    Location braceLoc = location();
    if (!match(TOKEN_LBRACE)) {
      expected("'{'");
      return nullptr;
    }
    NodeListBuilder cases(_alloc);
    while (!match(TOKEN_RBRACE)) {
      if (_token == TOKEN_END) {
        diag.error(braceLoc) << "Incomplete switch.";
      }

      Node::Kind kind = Node::Kind::CASE;
      NodeListBuilder caseValues(_alloc);
      if (match(TOKEN_ELSE)) {
        // 'else' block
        kind = Node::Kind::ELSE;
      } else {
        // case block
        for (;;) {
          if (_token == TOKEN_END) {
            diag.error(braceLoc) << "Incomplete switch.";
          }
          auto c = caseExpr();
          if (c == nullptr) {
            return nullptr;
          }
          caseValues.append(c);
          if (!match(TOKEN_COMMA)) {
            break;
          }
        }
      }

      if (!match(TOKEN_FAT_ARROW)) {
        expected("'=>'");
        return nullptr;
      }

      // Case block body
      auto body = caseBody();
      if (body == nullptr) {
        return nullptr;
      }

      ast::Oper* caseSt = new (_alloc) ast::Oper(kind, caseValues.location(), caseValues.build());
      caseSt->op = body;
      cases.append(caseSt);
    }
    return new (_alloc) ast::Control(Node::Kind::SWITCH, loc, test, cases.build());
  }

  Node* Parser::caseExpr() {
    auto e = primary();
    if (e != nullptr && match(TOKEN_RANGE)) {
      auto e2 = primary();
      if (e2 == nullptr) {
        return nullptr;
      }
      NodeListBuilder rangeBounds(_alloc);
      rangeBounds.append(e);
      rangeBounds.append(e2);
      return new (_alloc) ast::Oper(Node::Kind::RANGE, rangeBounds.location(), rangeBounds.build());
    }
    return e;
  }

  Node* Parser::caseBody() {
    // Case block body
    Node* body;
    if (_token == TOKEN_LBRACE) {
      return block();
    } else {
      body = expression();
      if (body != nullptr && !match(TOKEN_SEMI)) {
        expected("';'");
        return nullptr;
      }
    }
    return body;
  }

  Node* Parser::matchStmt() {
    Location loc = location();
    next();
    auto test = expression();
    if (test == nullptr) {
      return nullptr;
    }
    Location braceLoc = location();
    if (!match(TOKEN_LBRACE)) {
      expected("'{'");
      return nullptr;
    }

    NodeListBuilder patterns(_alloc);
    while (!match(TOKEN_RBRACE)) {
      if (_token == TOKEN_END) {
        diag.error(braceLoc) << "Incomplete match statement.";
      }

      NodeListBuilder patternArgs(_alloc);
      Location patternLoc = location();
      Node::Kind kind;
      if (match(TOKEN_ELSE)) {
        kind = Node::Kind::ELSE;
      } else if (_token == TOKEN_ID) {
        kind = Node::Kind::PATTERN;
        auto name = &Node::ABSENT;
        auto type = typeTerm();
        if (type && type->kind == Node::Kind::IDENT && match(TOKEN_COLON)) {
          name = type;
          type = typeTerm();
          if (type == nullptr) {
            return nullptr;
          }
        }
        patternArgs.append(name);
        patternArgs.append(type);
      } else {
        auto type = typeTerm(false);
        if (type == nullptr) {
          diag.error(location()) << "Match pattern expected.";
          return nullptr;
        }
        patternArgs.append(&Node::ABSENT);
        patternArgs.append(type);
      }

      if (!match(TOKEN_FAT_ARROW)) {
        expected("'=>'");
        return nullptr;
      }

      // Pattern block body
      auto body = caseBody();
      if (body == nullptr) {
        return nullptr;
      }
      patternArgs.append(body);
      ast::Oper* pattern = new (_alloc) ast::Oper(kind, patternLoc, patternArgs.build());
      patterns.append(pattern);
    }

    return new (_alloc) ast::Control(Node::Kind::MATCH, loc, test, patterns.build());
  }

  Node* Parser::tryStmt() {
    diag.error(location()) << "here";
    assert(false && "Implement tryStmt");
  }

  Node* Parser::returnStmt() {
    Location loc = location();
    next();
    Node* returnVal = nullptr;
    if (_token != TOKEN_SEMI && _token != TOKEN_LBRACE) {
      returnVal = exprList();
    }
    return new (_alloc) ast::UnaryOp(Node::Kind::RETURN, loc, returnVal);
  }

  Node* Parser::throwStmt() {
    Location loc = location();
    next();
    auto ex = expression();
    return new (_alloc) ast::UnaryOp(Node::Kind::THROW, loc, ex);
  }

  #if 0
    # All statements that end with a closing brace
    def p_closing_brace_stmt(self, p):
      '''closing_brace_stmt : block
                            | if_stmt
                            | while_stmt
                            | loop_stmt
                            | for_in_stmt
                            | for_stmt
                            | switch_stmt
                            | match_stmt
                            | try_stmt'''
      p[0] = p[1]

    def p_try_stmt(self, p):
      '''try_stmt : TRY block catch_list finally'''
      p[0] = ast.Try()

    def p_catch_list(self, p):
      '''catch_list : catch_list catch
                    | empty'''

    def p_catch(self, p):
      '''catch : CATCH ID COLON type_expr block
              | CATCH type_expr block
              | CATCH block'''

    def p_finally(self, p):
      '''finally : FINALLY block
                | empty'''

    def p_assign_stmt(self, p):
      '''assign_stmt : expr_or_tuple ASSIGN assign_stmt
                    | expr_or_tuple ASSIGN if_stmt
                    | expr_or_tuple ASSIGN expr_or_tuple'''
      p[0] = ast.Assign(location = self.location(p, 1, 3))
      p[0].setLeft(p[1])
      p[0].setRight(p[3])

  #endif

  // Expressions

  Node* Parser::expression() {
    return binary();
  }

  Node* Parser::exprList() {
    auto expr = binary();
    if (match(TOKEN_COMMA)) {
      NodeListBuilder builder(_alloc);
      builder.append(expr);
      while (_token != TOKEN_END) {
        expr = binary();
        if (expr == nullptr) {
          return nullptr;
        }
        builder.append(expr);
        if (!match(TOKEN_COMMA)) {
          break;
        }
      }
      return new (_alloc) ast::Oper(Node::Kind::TUPLE_TYPE, builder.location(), builder.build());
    }
    return expr;
  }

  Node* Parser::binary() {
    auto e0 = unary();
    if (e0 == nullptr) {
      return nullptr;
    }

    OperatorStack opstack(e0, _alloc);
    for (;;) {
      switch (_token) {
        case TOKEN_PLUS:
          opstack.pushOperator(Node::Kind::ADD, PREC_ADD_SUB);
          next();
          break;

        case TOKEN_MINUS:
          opstack.pushOperator(Node::Kind::SUB, PREC_ADD_SUB);
          next();
          break;

        case TOKEN_MUL:
          opstack.pushOperator(Node::Kind::MUL, PREC_MUL_DIV);
          next();
          break;

        case TOKEN_DIV:
          opstack.pushOperator(Node::Kind::DIV, PREC_MUL_DIV);
          next();
          break;

        case TOKEN_MOD:
          opstack.pushOperator(Node::Kind::MOD, PREC_MUL_DIV);
          next();
          break;

        case TOKEN_AMP:
          opstack.pushOperator(Node::Kind::BIT_AND, PREC_BIT_AND);
          next();
          break;

        case TOKEN_VBAR:
          opstack.pushOperator(Node::Kind::BIT_OR, PREC_BIT_OR);
          next();
          break;

        case TOKEN_CARET:
          opstack.pushOperator(Node::Kind::BIT_XOR, PREC_BIT_XOR);
          next();
          break;

        case TOKEN_AND:
          opstack.pushOperator(Node::Kind::LOGICAL_AND, PREC_LOGICAL_AND);
          next();
          break;

        case TOKEN_OR:
          opstack.pushOperator(Node::Kind::LOGICAL_OR, PREC_LOGICAL_OR);
          next();
          break;

        case TOKEN_LSHIFT:
          opstack.pushOperator(Node::Kind::LSHIFT, PREC_SHIFT);
          next();
          break;

        case TOKEN_RSHIFT:
          opstack.pushOperator(Node::Kind::RSHIFT, PREC_SHIFT);
          next();
          break;

          case TOKEN_RANGE:
          opstack.pushOperator(Node::Kind::RANGE, PREC_RANGE);
          next();
          break;

        case TOKEN_EQ:
          opstack.pushOperator(Node::Kind::EQUAL, PREC_RELATIONAL);
          next();
          break;

        case TOKEN_NE:
          opstack.pushOperator(Node::Kind::NOT_EQUAL, PREC_RELATIONAL);
          next();
          break;

        case TOKEN_REF_EQ:
          opstack.pushOperator(Node::Kind::REF_EQUAL, PREC_RELATIONAL);
          next();
          break;

        case TOKEN_LT:
          opstack.pushOperator(Node::Kind::LESS_THAN, PREC_RELATIONAL);
          next();
          break;

        case TOKEN_GT:
          opstack.pushOperator(Node::Kind::GREATER_THAN, PREC_RELATIONAL);
          next();
          break;

        case TOKEN_LE:
          opstack.pushOperator(Node::Kind::LESS_THAN_OR_EQUAL, PREC_RELATIONAL);
          next();
          break;

        case TOKEN_GE:
          opstack.pushOperator(Node::Kind::GREATER_THAN_OR_EQUAL, PREC_RELATIONAL);
          next();
          break;

  //       case TOKEN_RETURNS: {
  //         opstack.pushOperator(Node::Kind::RETURNS, PREC_RETURNS);
  //         next();
  //         break;
  //       }

        case TOKEN_AS: {
          // The second argument to 'as' is a type expression.
          opstack.pushOperator(Node::Kind::AS_TYPE, PREC_IS_AS);
          next();
          auto t1 = typeExpression();
          if (t1 == nullptr) {
            return nullptr;
          }
          opstack.pushOperand(t1);
          continue;
        }

        case TOKEN_IS: {
          next();

          // Handle 'is not'
          if (match(TOKEN_NOT)) {
            opstack.pushOperator(Node::Kind::IS_NOT, PREC_IS_AS);
          } else {
            opstack.pushOperator(Node::Kind::IS, PREC_IS_AS);
          }

          // Second argument is a type
          auto t1 = typeExpression();
          if (t1 == nullptr) {
            return nullptr;
          }
          opstack.pushOperand(t1);
          continue;
        }

        case TOKEN_COLON: {
          // Used is for tuples that are actually parameter lists.
          opstack.pushOperator(Node::Kind::EXPR_TYPE, PREC_RANGE);
          next();
          auto t1 = typeExpression();
          if (t1 == nullptr) {
            return nullptr;
          }
          opstack.pushOperand(t1);
          continue;
        }

        case TOKEN_IN:
          opstack.pushOperator(Node::Kind::IN, PREC_IN);
          next();
          break;

        case TOKEN_FAT_ARROW:
          opstack.pushOperator(Node::Kind::LAMBDA, PREC_FAT_ARROW);
          next();
          break;

        case TOKEN_NOT: {
          // Negated operators
          next();
          Location loc = location();
          if (match(TOKEN_IN)) {
            opstack.pushOperator(Node::Kind::NOT_IN, PREC_IN);
          } else {
            diag.error(loc) << "'in' expected after 'not'";
          }
          break;
        }

        default:
          goto done;
      }

      auto e1 = unary();
      if (e1 == nullptr) {
        return nullptr;
      }
      opstack.pushOperand(e1);
    }

  done:
    if (!opstack.reduceAll()) {
      return e0;
    }

    return opstack.expression();
  }

  #if 0
    def p_fluent_member_ref(self, p):
      '''fluent_member_ref : call_op DOT LBRACE RBRACE'''
      p[0] = ast.FluentMember(location = self.location(p, 1, 3))
      p[0].setBase(p[1])
      p[0].setName(p[3])
  #endif

  Node* Parser::unary() {
    Location loc = location();
    if (match(TOKEN_MINUS)) {
      auto expr = primary();
      if (expr == nullptr) {
        return nullptr;
      }
      return new (_alloc) ast::UnaryOp(Node::Kind::NEGATE, loc | expr->location, expr);
    } else if (match(TOKEN_PLUS)) {
      return primary();
    } else if (match(TOKEN_INC)) {
      auto expr = primary();
      if (expr == nullptr) {
        return nullptr;
      }
      return new (_alloc) ast::UnaryOp(Node::Kind::PRE_INC, loc | expr->location, expr);
    } else if (match(TOKEN_DEC)) {
      auto expr = primary();
      if (expr == nullptr) {
        return nullptr;
      }
      return new (_alloc) ast::UnaryOp(Node::Kind::PRE_DEC, loc | expr->location, expr);
    } else if (match(TOKEN_NOT)) {
      auto expr = primary();
      if (expr == nullptr) {
        return nullptr;
      }
      return new (_alloc) ast::UnaryOp(Node::Kind::LOGICAL_NOT, loc | expr->location, expr);
    }

    auto expr = primary();
    if (expr == nullptr) {
      return nullptr;
    }
    if (match(TOKEN_INC)) {
      return new (_alloc) ast::UnaryOp(Node::Kind::POST_INC, loc | expr->location, expr);
    } else if (match(TOKEN_DEC)) {
      return new (_alloc) ast::UnaryOp(Node::Kind::POST_DEC, loc | expr->location, expr);
    } else {
      return expr;
    }
  }

  Node* Parser::primary() {
    switch (_token) {
      case TOKEN_LPAREN: {
        NodeListBuilder builder(_alloc);
        Location loc = location();
        next();
        Location finalLoc = loc;
        bool trailingComma = true;
        if (!match(TOKEN_RPAREN)) {
          for (;;) {
            auto n = expression();
            if (Node::isError(n)) {
              return n;
            }
            builder.append(n);
            finalLoc = location();
            if (match(TOKEN_RPAREN)) {
              trailingComma = false;
              break;
            } else if (!match(TOKEN_COMMA)) {
              expected("',' or ')'");
            } else if (match(TOKEN_RPAREN)) {
              trailingComma = true;
              break;
            }
          }
        }
        loc = loc | finalLoc;
        if (builder.size() == 1 && !trailingComma) {
          return builder[0];
        }
        return new (_alloc) ast::Oper(Node::Kind::TUPLE_TYPE, loc, builder.build());
      }
      case TOKEN_TRUE: return node(Node::Kind::BOOLEAN_TRUE);
      case TOKEN_FALSE: return node(Node::Kind::BOOLEAN_FALSE);
      // case TOKEN_NULL: return node(Node::Kind::NULL_LITERAL);
      case TOKEN_STRING_LIT: return stringLit();
      case TOKEN_CHAR_LIT: return charLit();
      case TOKEN_DEC_INT_LIT:
      case TOKEN_HEX_INT_LIT: return integerLit();
      case TOKEN_FLOAT_LIT: return floatLit();

      case TOKEN_ID:
      case TOKEN_SELF:
      case TOKEN_SUPER:
      case TOKEN_VOID:
      case TOKEN_BOOL:
      case TOKEN_CHAR:
      case TOKEN_I8:
      case TOKEN_I16:
      case TOKEN_I32:
      case TOKEN_I64:
      case TOKEN_INT:
      case TOKEN_U8:
      case TOKEN_U16:
      case TOKEN_U32:
      case TOKEN_U64:
      case TOKEN_UINT:
      case TOKEN_FLOAT32:
      case TOKEN_FLOAT64: {
        auto e = namedPrimary();
        if (e == nullptr) {
          return nullptr;
        }
        return primarySuffix(e);
      }

      case TOKEN_IF: return ifStmt();
      case TOKEN_SWITCH: return switchStmt();
      case TOKEN_MATCH: return matchStmt();
      case TOKEN_TRY: return tryStmt();

      case TOKEN_DOT: {
        next();
        if (_token == TOKEN_ID) {
          auto expr = id();
          if (expr == nullptr) {
            return nullptr;
          }
          auto e = new (_alloc) ast::UnaryOp(Node::Kind::SELF_NAME_REF, expr->location, expr);
          return primarySuffix(e);
        } else {
          expected("identifier");
          return nullptr;
        }
      }

      default:
        diag.error(location()) << "Expression expected, not: " << _token << ".";
        break;
    }
    return nullptr;
  }

  Node* Parser::namedPrimary() {
    switch (_token) {
      case TOKEN_ID: return id();
      case TOKEN_SELF: {
        auto n = new (_alloc) Node(Node::Kind::SELF, location());
        next();
        return n;
      }
      case TOKEN_SUPER: {
        auto n = new (_alloc) Node(Node::Kind::SUPER, location());
        next();
        return n;
      }
      case TOKEN_VOID: return builtinType(ast::BuiltinType::VOID);
      case TOKEN_BOOL: return builtinType(ast::BuiltinType::BOOL);
      case TOKEN_CHAR: return builtinType(ast::BuiltinType::CHAR);
      case TOKEN_I8: return builtinType(ast::BuiltinType::I8);
      case TOKEN_I16: return builtinType(ast::BuiltinType::I16);
      case TOKEN_I32: return builtinType(ast::BuiltinType::I32);
      case TOKEN_I64: return builtinType(ast::BuiltinType::I64);
      case TOKEN_INT: return builtinType(ast::BuiltinType::INT);
      case TOKEN_U8: return builtinType(ast::BuiltinType::U8);
      case TOKEN_U16: return builtinType(ast::BuiltinType::U16);
      case TOKEN_U32: return builtinType(ast::BuiltinType::U32);
      case TOKEN_U64: return builtinType(ast::BuiltinType::U64);
      case TOKEN_UINT: return builtinType(ast::BuiltinType::UINT);
      case TOKEN_FLOAT32: return builtinType(ast::BuiltinType::F32);
      case TOKEN_FLOAT64: return builtinType(ast::BuiltinType::F64);
      default: assert(false);
    }
  }

  Node* Parser::primarySuffix(Node* expr) {
    Location openLoc = expr->location;
    while (_token != TOKEN_END) {
      if (match(TOKEN_DOT)) {
        if (_token == TOKEN_ID) {
          expr = new (_alloc) ast::MemberRef(openLoc | location(), copyOf(tokenValue()), expr);
          next();
        } else {
          expected("identifier");
        }
      } else if (match(TOKEN_LPAREN)) {
        NodeListBuilder args(_alloc);
        Location callLoc = openLoc;
        if (!callingArgs(args, callLoc)) {
          return nullptr;
        }
        ast::Oper* call = new (_alloc) ast::Oper(Node::Kind::CALL, callLoc, args.build());
        call->op = expr;
        expr = call;
      } else if (match(TOKEN_LBRACKET)) {
        NodeListBuilder typeArgs(_alloc);
        Location fullLoc = openLoc;
        if (!match(TOKEN_RBRACKET)) {
          for (;;) {
            if (_token == TOKEN_END) {
              diag.error(openLoc) << "Unmatched bracket.";
              return nullptr;
            }
            auto arg = expression();
            if (arg == nullptr) {
              return nullptr;
            }
            typeArgs.append(arg);
            fullLoc |= location();
            if (match(TOKEN_RBRACKET)) {
              break;
            } else if (!match(TOKEN_COMMA)) {
              expected("',' or ']'");
              return nullptr;
            }
          }
        }
        ast::Oper* spec = new (_alloc) ast::Oper(Node::Kind::SPECIALIZE, fullLoc, typeArgs.build());
        spec->op = expr;
        expr = spec;
      } else {
        return expr;
      }
    }
    return expr;
  }

  bool Parser::callingArgs(NodeListBuilder &args, Location& argsLoc) {
    if (!match(TOKEN_RPAREN)) {
      for (;;) {
        if (_token == TOKEN_END) {
          diag.error(argsLoc) << "Unmatched paren.";
          return false;
        }
        auto arg = expression();
        if (arg == nullptr) {
          return false;
        }

        // Keyword argument.
        if (match(TOKEN_ASSIGN)) {
          auto kwValue = expression();
          if (kwValue == nullptr) {
            return false;
          }
          NodeListBuilder kwArg(_alloc);
          kwArg.append(arg);
          kwArg.append(kwValue);
          arg = new (_alloc) ast::Oper(
              Node::Kind::KEYWORD_ARG, arg->location | kwValue->location, kwArg.build());
        }

        args.append(arg);
        argsLoc |= location();
        if (match(TOKEN_RPAREN)) {
          break;
        } else if (!match(TOKEN_COMMA)) {
          expected("',' or ')'");
          return false;
        }
      }
    }
    return true;
  }

  #if 0

    def p_tuple_expr(self, p):
      '''tuple_expr : LPAREN arg_list COMMA RPAREN
                    | LPAREN opt_arg_list RPAREN'''
      if len(p) == 5 or len(p[2]) != 1:
        p[0] = ast.Tuple(location = self.location(p, 1, 3))
        p[0].mutableArgs.extend(p[2])
      else:
        p[0] = p[2][0]

    def p_array_lit(self, p):
      '''array_lit : LBRACKET arg_list COMMA RBRACKET
                  | LBRACKET opt_arg_list RBRACKET'''
      p[0] = ast.ArrayLiteral(location = self.location(p, 1, 3))
      p[0].mutableArgs.extend(p[2])

  #endif

  /** Terminals */

  Node* Parser::dottedIdent() {
    auto result = id();
    if (result) {
      while (match(TOKEN_DOT)) {
        if (_token == TOKEN_ID) {
          result = new (_alloc) ast::MemberRef(
              result->location | location(), copyOf(tokenValue()), result);
          next();
        } else {
          expected("identifier");
        }
      }
    }
    return result;
  }

  llvm::StringRef Parser::dottedIdentStr() {
    llvm::SmallString<128> result;
    assert(_token == TOKEN_ID);
    result.append(tokenValue());
    next();
    while (match(TOKEN_DOT)) {
      result.push_back('.');
      if (_token == TOKEN_ID) {
        result.append(tokenValue());
        next();
      } else {
        expected("identifier");
      }
    }
    return copyOf(result);
  }

  Node* Parser::id() {
    assert(_token == TOKEN_ID);
    auto node = new (_alloc) ast::Ident(location(), copyOf(tokenValue()));
    next();
    return node;
  }

  Node* Parser::stringLit() {
    assert(_token == TOKEN_STRING_LIT);
    auto node = new (_alloc) ast::Literal(
        Node::Kind::STRING_LITERAL, location(), copyOf(tokenValue()));
    next();
    return node;
  }

  Node* Parser::charLit() {
    assert(_token == TOKEN_CHAR_LIT);
    auto node = new (_alloc) ast::Literal(
        Node::Kind::CHAR_LITERAL, location(), copyOf(tokenValue()));
    next();
    return node;
  }

  Node* Parser::integerLit() {
    assert(_token == TOKEN_DEC_INT_LIT || _token == TOKEN_HEX_INT_LIT);
    // bool uns = false;
    // int64_t value;
    // if (_token == TOKEN_DEC_INT_LIT) {
    //   value = strtoll(_lexer.tokenValue().c_str(), nullptr, 10);
    // } else {
    //   value = strtoll(_lexer.tokenValue().c_str(), nullptr, 16);
    // }
    // if (_lexer.tokenSuffix().empty()) {
    //   for (char ch : _lexer.tokenSuffix()) {
    //     assert(ch == 'u');
    //     uns = true;
    //   }
    // }
    auto node = new (_alloc) ast::Literal(
      Node::Kind::INTEGER_LITERAL, location(), copyOf(tokenValue()), copyOf(_lexer.tokenSuffix()));
    next();
    return node;
  }

  Node* Parser::floatLit() {
    assert(_token == TOKEN_FLOAT_LIT);
    // double d = strtod(_lexer.tokenValue().c_str(), nullptr);
    auto node = new (_alloc) ast::Literal(
        Node::Kind::FLOAT_LITERAL, location(), copyOf(tokenValue()));
    next();
    return node;
  }

  StringRef Parser::copyOf(const StringRef& str) {
    auto data = static_cast<char *>(_alloc.Allocate(str.size(), 1));
    std::copy(str.begin(), str.end(), data);
    return StringRef((char*) data, str.size());
  }

  // TODO: Get rid of this
  Node* Parser::node(Node::Kind kind) {
    auto n = new (_alloc) Node(kind, location());
    next();
    return n;
  }

  void Parser::expected(const StringRef& tokens) {
    diag.error(location()) << "Expected " << tokens << ".";
  }

  bool Parser::skipUntil(const std::unordered_set<TokenType>& tokens) {
    while (_token != TOKEN_END) {
      if (tokens.find(_token) != tokens.end()) {
        return true;
      }
      next();
    }
    return false;
  }

  static const std::unordered_set<TokenType> DEFN_TOKENS = {
    TOKEN_CLASS, TOKEN_STRUCT, TOKEN_INTERFACE, TOKEN_ENUM, TOKEN_EXTEND,
    TOKEN_FN, TOKEN_UNDEF, TOKEN_OVERRIDE,
    TOKEN_CONST, TOKEN_LET, TOKEN_IMPORT,
  };

  bool Parser::skipOverDefn() {
    int nesting = 0;
    while (_token != TOKEN_END) {
      if (match(TOKEN_LBRACE)) {
        nesting += 1;
      } else if (match(TOKEN_RBRACE)) {
        if (nesting > 0) {
          nesting -= 1;
        }
      } else if (nesting == 0 && DEFN_TOKENS.find(_token) != DEFN_TOKENS.end()) {
        return true;
      }
      next();
    }
    return false;
  }

  #if 0
    def takeComment(self, defn):
      if self.lexer.commentLines:
        dc = graph.DocComment()
        dc.setLocation(self.lexer.commentLoc)
        dc.setColumn(self.lexer.commentColumn)
        dc.getMutableText().extend(self.lexer.commentLines)
        self.lexer.commentLines = []
        defn.setDocComment(dc)

    def noComment(self):
      if self.lexer.commentLines:
  #       self.errorReporter.errorAt(self.lexer.commentLoc, "Doc comment ignored.")
        self.lexer.commentLines = []
  #endif

  void OperatorStack::pushOperand(Node* operand) {
    assert(_entries.back().operand == nullptr);
    _entries.back().operand = operand;
  }

  bool OperatorStack::pushOperator(ast::Node::Kind oper, int16_t prec, bool rightAssoc) {
    assert(_entries.back().operand != nullptr);
    if (!reduce(prec, rightAssoc)) {
      return false;
    }

    _entries.push_back(Entry());
    _entries.back().oper = oper;
    _entries.back().precedence = prec;
    _entries.back().rightAssoc = rightAssoc;
    _entries.back().operand = nullptr;
    return true;
  }

  bool OperatorStack::reduce(int16_t precedence, bool rightAssoc) {
    while (_entries.size() > 1) {
      Entry back = _entries.back();
      if (back.precedence < precedence) {
        break;
      }
      assert(back.operand != nullptr);
      _entries.pop_back();
      NodeListBuilder args(_alloc);
      args.append(_entries.back().operand);
      args.append(back.operand);
      Location loc = _entries.back().operand->location | back.operand->location;
      auto combined = new (_alloc) ast::Oper(back.oper, loc, args.build());
      _entries.back().operand = combined;
    }
    return true;
  }

  bool OperatorStack::reduceAll() {
    if (!reduce(0, 0)) {
      return false;
    }
    assert(_entries.size() == 1);
    return true;
  }
}
