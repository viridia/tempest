#ifndef TEMPEST_PARSE_PARSER_HPP
#define TEMPEST_PARSE_PARSER_HPP 1

#ifndef TEMPEST_PARSE_LEXER_HPP
  #include "tempest/parse/lexer.hpp"
#endif

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

#ifndef TEMPEST_AST_DEFN_HPP
  #include "tempest/ast/defn.hpp"
#endif

#ifndef TEMPEST_AST_IDENT_HPP
  #include "tempest/ast/ident.hpp"
#endif

#ifndef LLVM_SUPPORT_ALLOCATOR_H
  #include <llvm/Support/Allocator.h>
#endif

#include <unordered_set>

namespace tempest::ast {
  class Defn;
  class Module;
}

namespace tempest::error {
  class Reporter;
}

namespace tempest::parse {
  using tempest::source::DocComment;
  using tempest::source::ProgramSource;
  using tempest::source::Location;
  using tempest::error::Reporter;

  class NodeListBuilder;

  enum DeclarationScope {
    DECL_GLOBAL,
    DECL_MEMBER,
  };

  /** Stack used for operator precedence parsing. */
  class OperatorStack {
  public:
    /** Contains an operator/operand pair. The bottom element of the stack contains only an
        operand:
        [NULL value][op value][op value] ...
    */
    struct Entry {
      ast::Node* operand;
      ast::Node::Kind oper;
      int16_t precedence;
      bool rightAssoc;

      Entry()
        : operand(nullptr)
        , oper(ast::Node::Kind::ABSENT)
        , precedence(0)
        , rightAssoc(false)
      {}

      Entry(const Entry& entry)
        : operand(entry.operand)
        , oper(entry.oper)
        , precedence(entry.precedence)
        , rightAssoc(entry.rightAssoc)
      {}
    };

    OperatorStack(ast::Node* initialExpr, llvm::BumpPtrAllocator& alloc)
      : _alloc(alloc)
    {
      _entries.push_back(Entry());
      _entries.back().operand = initialExpr;
    }

    void pushOperand(ast::Node* operand);
    bool pushOperator(ast::Node::Kind oper, int16_t prec, bool rightAssoc = false);
    bool reduce(int16_t precedence, bool rightAssoc = false);
    bool reduceAll();

    ast::Node* expression() const {
      return _entries.front().operand;
    }
  private:
    llvm::BumpPtrAllocator& _alloc;
    std::vector<Entry> _entries;
  };

  /** Spark source parser. */
  class Parser {
  public:
    Parser(ProgramSource* source, llvm::BumpPtrAllocator& alloc);
    Parser(const Parser&) = delete;

    bool done() {
      return _token == TokenType::TOKEN_END;
    }

    ast::Module* module();
    ast::Defn* declaration(DeclarationScope scope);
    ast::Node* expression();
    ast::Node* typeExpression();
  private:
    ProgramSource* _source;
    llvm::BumpPtrAllocator& _alloc;
    Lexer _lexer;
    TokenType _token;
    TokenType _prevToken;
    bool _recovering;

    ast::Node* importStmt(ast::Node::Kind kind);
    bool memberDeclaration(NodeListBuilder& decls);
    ast::Node* attribute();
    // llvm::StringRef methodName();
    ast::Defn* compositeTypeDef();
    bool classBody(ast::TypeDefn* d);
    bool classMember(NodeListBuilder &members, NodeListBuilder &friends);
    ast::Defn* enumTypeDef();
    ast::Defn* aliasTypeDef();
    bool enumMember(NodeListBuilder &members);
    ast::Defn* methodDef();
    ast::Node* methodBody();
    void templateParamList(NodeListBuilder& params);
    ast::TypeParameter* templateParam();

    bool requirements(NodeListBuilder& requires);
    ast::Node* requirement();
    ast::Node* requireBinaryOp(ast::Node::Kind kind, ast::Node* left);
    ast::Node* requireCall(ast::Node::Kind kind, ast::Node* fn);
    bool paramList(NodeListBuilder& params);

    ast::Defn* fieldDef();
    ast::ValueDefn* varDeclList(ast::Node::Kind kind);
    ast::ValueDefn* varDecl(ast::Node::Kind kind);

    ast::Node* typeUnion();
    ast::Node* typeTerm(bool allowPartial = false);
    ast::Node* typePrimary(bool allowPartial = false);
    ast::Node* functionType();
    ast::Node* baseTypeName();
    ast::Node* enumBaseTypeName();
    ast::Node* specializedTypeName();
    ast::Node* builtinType(ast::BuiltinType::Type t);

    ast::Node* exprList();
    ast::Node* binary();
    ast::Node* unary();
    ast::Node* primary();
    ast::Node* namedPrimary();
    ast::Node* primarySuffix(ast::Node* expr);
    bool callingArgs(NodeListBuilder &args, source::Location& argsLoc);

    ast::Node* block();
    ast::Node* requiredBlock();
    ast::Node* stmt();
    ast::Node* assignStmt();
    ast::Node* ifStmt();
    ast::Node* whileStmt();
    ast::Node* loopStmt();
    ast::Node* forStmt();
    ast::Node* switchStmt();
    ast::Node* caseExpr();
    ast::Node* caseBody();
    ast::Node* matchStmt();
    ast::Node* tryStmt();
    ast::Node* returnStmt();
    ast::Node* throwStmt();
    ast::Node* localDefn();

    ast::Node* dottedIdent();
    llvm::StringRef dottedIdentStr();
    ast::Node* id();
    ast::Node* stringLit();
    ast::Node* charLit();
    ast::Node* integerLit();
    ast::Node* floatLit();

    /** Read the next token. */
    void next();

    /** Match a token. */
    bool match(TokenType tok);

    /** Location of current token. */
    const Location& location() const { return _lexer.tokenLocation(); }

    /** String value of current token. */
    const std::string& tokenValue() const { return _lexer.tokenValue(); }

    /** Make a copy of this string within the current alloc. */
    llvm::StringRef copyOf(const llvm::StringRef& str);

    /** Print an error message about expected tokens. */
    void expected(const llvm::StringRef& tokens);

    /** Skip until one of the given tokens is encountered. */
    bool skipUntil(const std::unordered_set<TokenType>& tokens);

    /** Skip to the start of the next defn. */
    bool skipOverDefn();

    /** Create a new node with the given kind and the current token location. Also consume the
        current token. */
    ast::Node* node(ast::Node::Kind kind);
  };

}

#endif
