#ifndef TEMPEST_SEMA_GRAPH_DEFN_H
#define TEMPEST_SEMA_GRAPH_DEFN_H 1

#ifndef TEMPEST_GRAPH_SYMBOLTABLE_H
  #include "tempest/sema/graph/symboltable.h"
#endif

namespace tempest::sema::graph {
  class Expr;
  class Type;
  class TypeVar;
  class TypeParameter;
  class FunctionType;
  typedef llvm::ArrayRef<Type*> TypeArray;

  enum Visibility {
    PUBLIC,
    PROTECTED,
    PRIVATE,
    INTERNAL,
  };

  /** Indicates that the type definition is a builtin type .*/
  enum class IntrinsicType {
    NONE = 0,
    OBJECT_CLASS,
  };

  /** Base class for semantic graph definitions. */
  class Defn : public Member {
  public:
    Defn(
        Kind kind,
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn = nullptr)
      : Member(kind, name)
      , _definedIn(definedIn)
      , _location(location)
      , _visibility(PUBLIC)
      , _final(false)
      , _override(false)
      , _abstract(false)
      , _undef(false)
      , _static(false)
      , _overloadIndex(0)
    {}

    /** Source location where this was defined. */
    const source::Location& location() const { return _location; }

    /** The scope in which this definition was defined. */
    Member* definedIn() const { return _definedIn; }

    /** Visibility of this symbol. */
    Visibility visibility() const { return _visibility; }
    void setVisibility(Visibility visibility) { _visibility = visibility; }

    /** Whether this definition has the 'static' modifier. */
    bool isStatic() const { return _static; }
    void setStatic(bool value) { _static = value; }

    /** Whether this definition has the 'override' modifier. */
    bool isOverride() const { return _override; }
    void setOverride(bool value) { _override = value; }

    /** Whether this definition has the 'undef' modifier. */
    bool isUndef() const { return _undef; }
    void setUndef(bool value) { _undef = value; }

    /** If true, this definition can't be overridden. */
    bool isFinal() const { return _final; }
    void setFinal(bool value) { _final = value; }

    /** If true, this definition can't be instantiated. */
    bool isAbstract() const { return _abstract; }
    void setAbstract(bool value) { _abstract = value; }

    /** The list of attributes on this definition. */
    std::vector<Expr*>& attributes() { return _attributes; }
    const std::vector<Expr*>& attributes() const { return _attributes; }

    /** The list of all type variables that could afffect this definition, including type
        variables defined by enclosing scopes. These are always in order from outermost
        scope to innermost. */
    std::vector<TypeVar*>& typeVars() { return _typeVars; }
    const std::vector<TypeVar*>& typeVars() const { return _typeVars; }

    // If non-zero, means this is the Nth member with the same name.
    int32_t overloadIndex() const { return _overloadIndex; }
    void setOverloadIndex(int32_t index) { _overloadIndex = index; }

    // Defn* asDefn() final { return this; }

    static bool classof(const Defn* m) { return true; }
    static bool classof(const Member* m) {
      return m->kind == Kind::TYPE
          || m->kind == Kind::TYPE_PARAM
          || m->kind == Kind::CONST_DEF
          || m->kind == Kind::LET_DEF
          || m->kind == Kind::FUNCTION
          || m->kind == Kind::FUNCTION_PARAM;
    }

  protected:
    void formatModifiers(std::ostream& out) const;

  protected:
    Member* _definedIn;
    source::Location _location;
    Visibility _visibility;
    bool _final;
    bool _override;               // Overridden method
    bool _abstract;
    bool _undef;                  // Undefined method
    bool _static;                 // Was declared static

    std::vector<Expr*> _attributes;
    std::vector<TypeVar*> _typeVars;

    int32_t _overloadIndex;
  //  docComment: DocComment = 7;   # Doc comments
  };

  /** A definition that may or may not have template parameters. */
  class GenericDefn : public Defn {
  public:
    GenericDefn(
        Kind kind,
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn)
      : Defn(kind, location, name, definedIn)
      , _typeParamScope(std::make_unique<SymbolTable>())
    {
    }

    ~GenericDefn();

    /** List of template parameters. */
    std::vector<TypeParameter*>& typeParams() { return _typeParams; }
    const std::vector<TypeParameter*>& typeParams() const { return _typeParams; }

    /** List of all template parameters including the ones from enclosing scopes. */
    std::vector<TypeParameter*>& allTypeParams() { return _allTypeParams; }
    const std::vector<TypeParameter*>& allTypeParams() const { return _allTypeParams; }

    /** Scope containing all of the type parameters of this type. */
    SymbolTable* typeParamScope() const { return _typeParamScope.get(); }

    /** List of required functions. */
    // std::vector<RequiredFunction*>& requiredFunctions() { return _requiredFunctions; }
    // const std::vector<RequiredFunction*>& requiredFunctions() const { return _requiredFunctions; }

    /** Scope containing all of the type parameters of this type. */
    // SymbolTable* requiredMethodScope() const { return _requiredMethodScope.get(); }

    /** Scopes searched when resolving a reference to a qualified name that is mentioned in a
        'where' clause. So for example, if a template has a constraint such as
        'where hashing.hash(T)', then these scopes intercept the symbol lookup of 'hashing.hash'
        and replace it with the required function placeholder. */
    // std::unordered_map<Member*, SymbolTable*>& interceptScopes() { return _interceptScopes; }
    // const std::unordered_map<Member*, SymbolTable*>& interceptScopes() const {
    //   return _interceptScopes;
    // }

    /** Dynamic casting support. */
    static bool classof(const GenericDefn* m) { return true; }
    static bool classof(const Member* m) {
      return m->kind == Kind::TYPE || m->kind == Kind::FUNCTION;
    }

  private:
    std::vector<TypeParameter*> _typeParams;
    std::vector<TypeParameter*> _allTypeParams;
    std::unique_ptr<SymbolTable> _typeParamScope;
    // std::unique_ptr<SymbolTable> _requiredMethodScope;
    // std::unordered_map<Member*, SymbolTable*> _interceptScopes;
  };

  /** A type definition. */
  class TypeDefn : public GenericDefn {
  public:
    TypeDefn(
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn = nullptr)
      : GenericDefn(Kind::TYPE, location, name, definedIn)
      , _memberScope(std::make_unique<SymbolTable>())
    {
    }

    ~TypeDefn();

    /** The type defined by this type definition. */
    Type* type() const { return _type; }
    void setType(Type* type) { _type = type; }

    /** List of all members of this type. */
    std::vector<Defn*>& members() { return _members; }
    const std::vector<Defn*>& members() const { return _members; }

    /** Scope containing all of the members of this type. */
    SymbolTable* memberScope() const { return _memberScope.get(); }

    /** If this type is an intrinsic type, here is the information for it. */
    IntrinsicType intrinsic() const { return _intrinsic; }
    void setIntrinsic(IntrinsicType i) { _intrinsic = i; }

    // void format(std::ostream& out) const;

    /** Dynamic casting support. */
    static bool classof(const TypeDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::TYPE; }

  private:
    Type* _type;
    std::vector<Defn*> _members;
    std::unique_ptr<SymbolTable> _memberScope;
    IntrinsicType _intrinsic = IntrinsicType::NONE;

  //   # List of friend declarations for thibs class
  //   friends: list[Member] = 3;
  //
  //   # If this is an attribute type, some attribute properties.
  //   attribute: AttributeInfo = 15;
  };

  /** A type parameter of a function or composite type. */
  class TypeParameter : public Defn {
  public:
    TypeParameter(
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn = nullptr)
      : Defn(Kind::TYPE_PARAM, location, name, definedIn)
      , _valueType(nullptr)
      , _typeVar(nullptr)
      , _defaultType(nullptr)
      , _index(0)
      , _variadic(false)
    {}

    TypeParameter(
        const source::Location& location,
        llvm::StringRef name,
        int32_t index,
        Member* definedIn = nullptr)
      : Defn(Kind::TYPE_PARAM, location, name, definedIn)
      , _valueType(nullptr)
      , _typeVar(nullptr)
      , _defaultType(nullptr)
      , _index(index)
      , _variadic(false)
    {}

    /** If this type parameter represents a constant value rather than a type, then this is
        the type of that constant value. Otherwise, this is nullptr. */
    Type* valueType() const { return _valueType; }
    void setValueType(Type* type) { _valueType = type; }

    /** The type variable for this parameter. This is used whenever there is a need to
        refer to this parameter within a type expression. */
    TypeVar* typeVar() const { return _typeVar; }
    void setTypeVar(TypeVar* type) { _typeVar = type; }

    /** If this type pararameter holds a type expression, this is the default value for
        the type parameter. nullptr if no default has been specified. */
    Type* defaultType() const { return _defaultType; }
    void setDefaultType(Type* type) { _defaultType = type; }

    /** The ordinal index of this type parameter relative to other type parameters on
        the same definition. This includes type parameters defined in enclosing scopes,
        which precede type parameters in the current scope.

        Using the index, a specialization of a generic definition can be represented
        as a tuple, where the index of each type parameter is used to access the associated
        tuple member.
    */
    bool index() const { return _index; }
    void setIndex(int32_t index) { _index = index; }

    /** Indicates this type parameter accepts a list of types. */
    bool isVariadic() const { return _variadic; }
    void setVariadic(bool variadic) { _variadic = variadic; }

    /** Indicates this type parameter is tied to the 'self' type used to invoke the method or
        property that defines that parameter. */
    bool isSelfParam() const { return _selfParam; }
    void setSelfParam(bool selfParam) { _selfParam = selfParam; }

    /** Indicates this type parameter is tied to the class used to invoke the static method or
        property that defines that parameter. */
    bool isClassParam() const { return _classParam; }
    void setClassParam(bool classParam) { _classParam = classParam; }

    /** List of subtype constraints on this type parameter. The type bound to this parameter
        must be a subtype of every type listed. */
    const TypeArray& subtypeConstraints() const { return _subtypeConstraints; }
    void setSubtypeConstraints(const TypeArray& types) { _subtypeConstraints = types; }

    // void format(std::ostream& out) const;

    /** Dynamic casting support. */
    static bool classof(const TypeParameter* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::TYPE_PARAM; }

  private:
    Type* _valueType;
    TypeVar* _typeVar;
    Type* _defaultType;
    int32_t _index;
    bool _variadic;
    bool _selfParam;
    bool _classParam;
    TypeArray _subtypeConstraints;
  };

  /** Base class of let, const and enum value definitions. */
  class ValueDefn : public Defn {
  public:
    ValueDefn(
        Kind kind,
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn = nullptr)
      : Defn(kind, location, name, definedIn)
      , _type(nullptr)
      , _init(nullptr)
      , _fieldIndex(0)
      , _defined(true)
    {}

    /** Type of this value. */
    Type* type() const { return _type; }
    void setType(Type* type) { _type = type; }

    /** Initialization expression or default value. */
    Expr* init() const { return _init; }
    void setInit(Expr* init) { _init = init; }

    /** Field index for fields within an object. */
    int32_t fieldIndex() const { return _fieldIndex; }
    void setFieldIndex(int32_t index) { _fieldIndex = index; }

    /** Whether this variable is actually defined yet. This is used to detect uses of local
        variables which occur prior to the definition but are in the same block. */
    bool isDefined() const { return _defined; }
    void setDefined(bool defined) { _defined = defined; }

    // void format(std::ostream& out) const;

    /** Dynamic casting support. */
    static bool classof(const ValueDefn* m) { return true; }
    static bool classof(const Member* m) {
      return m->kind == Kind::LET_DEF ||
          m->kind == Kind::CONST_DEF ||
          m->kind == Kind::ENUM_VAL ||
          m->kind == Kind::FUNCTION_PARAM ||
          m->kind == Kind::TUPLE_MEMBER;
    }

  private:
    Type* _type;
    Expr* _init;
    int32_t _fieldIndex;
    bool _defined;
  };

  class EnumValueDefn : public ValueDefn {
  public:
    EnumValueDefn(
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn = nullptr)
      : ValueDefn(Kind::ENUM_VAL, location, name, definedIn)
      , _ordinal(0)
    {}

    /** Index of this enum value within the enumeration type. */
    int32_t ordinal() const { return _ordinal; }
    void setOrdinal(int32_t n) { _ordinal = n; }

    // void format(std::ostream& out) const;

    /** Dynamic casting support. */
    static bool classof(const EnumValueDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::ENUM_VAL; }

  private:
    int32_t _ordinal;
  };

  class ParameterDefn : public ValueDefn {
  public:
    ParameterDefn(
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn = nullptr)
      : ValueDefn(Kind::FUNCTION_PARAM, location, name, definedIn)
      , _internalType(nullptr)
      , _keywordOnly(false)
      , _selfParam(false)
      , _classParam(false)
      , _variadic(false)
      , _expansion(false)
    {}

    /** Type of this param within the body of the function, which may be different than the
        declared type of the parameter. Example: variadic parameters become arrays. */
    Type* internalType() const { return _internalType; }
    void setInternalType(Type* type) { _internalType = type; }

    /** Indicates a keyword-only parameter. */
    bool isKeywordOnly() const { return _keywordOnly; }
    void setKeywordOnly(bool keywordOnly) { _keywordOnly = keywordOnly; }

    /** Indicates this parameter accepts a list of values. */
    bool isVariadic() const { return _variadic; }
    void setVariadic(bool variadic) { _variadic = variadic; }

    /** Indicates this parameter's type is actually an expansion of a variadic template param. */
    bool isExpansion() const { return _expansion; }
    void setExpansion(bool expansion) { _expansion = expansion; }

    /** Indicates that this is the special 'self' parameter. */
    bool isSelfParam() const { return _selfParam; }
    void setSelfParam(bool selfParam) { _selfParam = selfParam; }

    /** Indicates that this is the special 'class' parameter. */
    bool isClassParam() const { return _classParam; }
    void setClassParam(bool classParam) { _classParam = classParam; }

    // void format(std::ostream& out) const;

    /** Dynamic casting support. */
    static bool classof(const ParameterDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::FUNCTION_PARAM; }

  private:
    Type* _internalType;
    bool _keywordOnly;
    bool _selfParam;
    bool _classParam;
  //   bool _mutable;
    bool _variadic;
    bool _expansion;

  //  explicit: bool = 3;           # No type conversion - type must be exact
  };

  /** A method or global function. */
  class FunctionDefn : public GenericDefn {
  public:
    FunctionDefn(
        const source::Location& location,
        llvm::StringRef name,
        Member* definedIn = nullptr)
      : GenericDefn(Kind::FUNCTION, location, name, definedIn)
      , _type(nullptr)
      , _paramScope(std::make_unique<SymbolTable>())
      // , _intrinsic(nullptr)
      , _selfType(nullptr)
      , _body(nullptr)
      , _constructor(false)
      , _requirement(false)
      , _native(false)
      , _methodIndex(0)
    {}

    ~FunctionDefn();

    /** Type of this function. */
    FunctionType* type() const { return _type; }
    void setType(FunctionType* type) { _type = type; }

    /** List of function parameters. */
    std::vector<ParameterDefn*>& params() { return _params; }
    const std::vector<ParameterDefn*>& params() const { return _params; }

    /** Scope containing all of the parameters of this function. */
    SymbolTable* paramScope() const { return _paramScope.get(); }

    /** The function body. nullptr if no body has been declared. */
    Type* selfType() const { return _selfType; }
    void setSelfType(Type* t) { _selfType = t; }

    /** The type of the 'self' parameter. */
    Expr* body() const { return _body; }
    void setBody(Expr* body) { _body = body; }

    /** List of all local variables and definitions. */
    std::vector<Defn*>& localDefns() { return _localDefns; }
    const std::vector<Defn*>& localDefns() const { return _localDefns; }

    /** True if this function is a constructor. */
    bool isConstructor() const { return _constructor; }
    void setConstructor(bool ctor) { _constructor = ctor; }

    /** True if this function is a requirement. */
    bool isRequirement() const { return _requirement; }
    void setRequirement(bool req) { _requirement = req; }

    /** True if this is a native function. */
    bool isNative() const { return _native; }
    void setNative(bool native) { _native = native; }

    /** If this type is an intrinsic type, here is the information for it. */
    // intrinsic::IntrinsicFunction* intrinsic() const { return _intrinsic; }
    // void setIntrinsic(intrinsic::IntrinsicFunction* i) { _intrinsic = i; }

    /** Method index for this function in a dispatch table. */
    int32_t methodIndex() const { return _methodIndex; }
    void setMethodIndex(int32_t index) { _methodIndex = index; }

    // void format(std::ostream& out) const;

    /** Dynamic casting support. */
    static bool classof(const FunctionDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::FUNCTION; }

  private:
    FunctionType* _type;
    std::vector<ParameterDefn*> _params;
    std::unique_ptr<SymbolTable> _paramScope;
    std::vector<Defn*> _localDefns;
    // intrinsic::IntrinsicFunction* _intrinsic;
    Type* _selfType;
    Expr* _body;
    bool _constructor;
    bool _requirement;
    bool _native;
    int32_t _methodIndex;
    //linkageName: string = 10;     # If present, indicates the symbolic linkage name of this function
    //evaluable : bool = 12;        # If true, function can be evaluated at compile time.
  };

  /** A specialized generic definition. */
  class SpecializedDefn : public Member {
  public:
    SpecializedDefn(
        GenericDefn* base,
        const llvm::ArrayRef<Type*>& typeArgs)
      : Member(Kind::SPECIALIZED, base->name())
      , _base(base)
      , _typeArgs(typeArgs.begin(), typeArgs.end())
    {
    }

    /** The generic type that this is a specialiation of. */
    GenericDefn* base() const { return _base; }

    /** The array of type arguments for this type. */
    const llvm::ArrayRef<Type*> typeArgs() const { return _typeArgs; }

    /** Dynamic casting support. */
    static bool classof(const SpecializedDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::SPECIALIZED; }

  private:
    GenericDefn* _base;
    llvm::SmallVector<Type*, 4> _typeArgs;
  };
}

#endif
