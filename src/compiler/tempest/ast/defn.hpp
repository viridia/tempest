#ifndef TEMPEST_AST_DEFN_HPP
#define TEMPEST_AST_DEFN_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

namespace tempest::common {
  class DocComment;
}

namespace tempest::ast {
  using llvm::StringRef;

  /** Base for all definitions. */
  class Defn : public Node {
  public:
    StringRef name;
    NodeList members;
    NodeList attributes;
    NodeList typeParams;
    NodeList requires;
    common::DocComment* docComment;
    bool variadicTemplate = false;

    Defn(Kind kind, const Location& location, const StringRef& name)
      : Node(kind, location)
      , name(name)
      , docComment(nullptr)
      , _private(false)
      , _protected(false)
      , _static(false)
      , _override(false)
      , _final(false)
      , _abstract(false)
      , _export(false)
    {}

    /** Whether this definition has the 'private' modifier. */
    bool isPrivate() const { return _private; }
    void setPrivate(bool value) { _private = value; }

    /** Whether this definition has the 'protected' modifier. */
    bool isProtected() const { return _protected; }
    void setProtected(bool value) { _protected = value; }

    /** Whether this definition has the 'static' modifier. */
    bool isStatic() const { return _static; }
    void setStatic(bool value) { _static = value; }

    /** Whether this definition has the 'override' modifier. */
    bool isOverride() const { return _override; }
    void setOverride(bool value) { _override = value; }

    /** Whether this definition has the 'final' modifier. */
    bool isFinal() const { return _final; }
    void setFinal(bool value) { _final = value; }

    /** Whether this definition has the 'abstract' modifier. */
    bool isAbstract() const { return _abstract; }
    void setAbstract(bool value) { _abstract = value; }

    /** Whether this definition has the 'export' modifier. */
    bool isExport() const { return _export; }
    void setExport(bool value) { _export = value; }

  private:

    bool _private;
    bool _protected;
    bool _static;
    bool _override;
    bool _final;
    bool _abstract;
    bool _export;
  };

  class TypeDefn : public Defn {
  public:
    NodeList extends;
    NodeList implements;
    NodeList friends;

    TypeDefn(Kind kind, const Location& location, const StringRef& name)
      : Defn(kind, location, name)
    {}
  };

  /** Base for all vars, lets, enum values and parameters. */
  class ValueDefn : public Defn {
  public:
    const Node* type;
    const Node* init;

    ValueDefn(Kind kind, const Location& location, const StringRef& name)
      : Defn(kind, location, name)
      , type(nullptr)
      , init(nullptr)
    {}
  };

  // class Var(ValueDefn): pass
  // class Let(ValueDefn): pass

  class EnumValue : public ValueDefn {
  public:
    int32_t ordinal;

    EnumValue(const Location& location, const StringRef& name)
      : ValueDefn(Kind::ENUM_VALUE, location, name)
    {}
  };

  class Parameter : public ValueDefn {
  public:
    bool keywordOnly = false;
    bool selfParam = false;
    bool classParam = false;
  //   bool _mutable;
    bool variadic = false;
    bool expansion = false;

    Parameter(const Location& location, const StringRef& name)
      : ValueDefn(Kind::PARAMETER, location, name)
    {}
  };

  // class Parameter(ValueDefn):
  //   explicit = defscalar(bool)      # No type conversion - type must be exact
  //   multi = defscalar(bool)         # Represents multiple params expanded from a tuple type

  class TypeParameter : public Defn {
  public:
    const Node* init;
    bool variadic;
    NodeList constraints;

    TypeParameter(const Location& location, const StringRef& name)
      : Defn(Kind::TYPE_PARAMETER, location, name)
      , init(nullptr)
      , variadic(false)
    {}
  };

  class Function : public Defn {
  public:
    const Node* returnType;
    NodeList params;
    const Node* body;
    bool constructor = false;
    bool native = false;
    bool getter = false;
    bool setter = false;
    bool mutableSelf = false;
    bool unsafe = false;
    bool variadic = false;
    const Node* selfType = nullptr;

    Function(const Location& location, const StringRef& name)
      : Defn(Kind::FUNCTION, location, name)
      , returnType(nullptr)
      , body(nullptr)
    {}
  };
}

#endif
