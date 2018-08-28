#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
#define TEMPEST_SEMA_GRAPH_DEFN_HPP 1

#ifndef TEMPEST_GRAPH_SYMBOLTABLE_HPP
  #include "tempest/sema/graph/symboltable.hpp"
#endif

namespace tempest::ast {
  class Function;
  class Parameter;
  class TypeDefn;
  class TypeParameter;
  class ValueDefn;
}

namespace tempest::sema::graph {
  using tempest::source::Locatable;

  class Expr;
  class Type;
  class TypeVar;
  class TypeParameter;
  class FunctionType;
  class SpecializedType;
  typedef llvm::ArrayRef<const Type*> TypeArray;

  enum Visibility {
    PUBLIC,
    PROTECTED,
    PRIVATE,
    INTERNAL,
  };

  /** Indicates that the type definition is a builtin type .*/
  enum class IntrinsicType {
    NONE = 0,

    // Built-in classes
    OBJECT_CLASS,

    // Build-in traits
    ADDITION_TRAIT,
    SUBTRACTION_TRAIT,
    MULTIPLICATION_TRAIT,
    DIVISION_TRAIT,
    MODULUS_TRAIT,
  };

  /** Base class for semantic graph definitions. */
  class Defn : public Member, public Locatable {
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
      , _resolving(false)
      , _resolved(false)
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

    /** Whether this was declared as a getter. */
    bool isGetter() const { return _getter; }
    void setGetter(bool value) { _getter = value; }

    /** Whether this was declared as a setter. */
    bool isSetter() const { return _setter; }
    void setSetter(bool value) { _setter = value; }

    /** Whether type resolution is being performed on this definition. */
    bool isResolving() const { return _resolving; }
    void setResolving(bool value) { _resolving = value; }

    /** Whether type resolution has been completed for this definition. */
    bool isResolved() const { return _resolved; }
    void setResolved(bool value) { _resolved = value; }

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

    /** Implemente Locatable. */
    const source::Location& getLocation() const { return location(); }

    // Defn* asDefn() final { return this; }

    static bool classof(const Defn* m) { return true; }
    static bool classof(const Member* m) {
      return m->kind == Kind::TYPE
          || m->kind == Kind::TYPE_PARAM
          || m->kind == Kind::CONST_DEF
          || m->kind == Kind::LET_DEF
          || m->kind == Kind::ENUM_VAL
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
    bool _getter;                 // Declared as 'get'
    bool _setter;                 // Declared as 'set'
    bool _resolving;              // Means type resolution pass in progress
    bool _resolved;               // Means type resolution finished

    std::vector<Expr*> _attributes;
    std::vector<TypeVar*> _typeVars;

    int32_t _overloadIndex;
  //  docComment: DocComment = 7;   # Doc comments
  };

  typedef std::vector<Defn*> DefnList;
  typedef llvm::ArrayRef<Defn*> DefnArray;

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

    /** Abstract syntax tree for this member. */
    const ast::TypeDefn* ast() const { return _ast; }
    void setAst(const ast::TypeDefn* ast) { _ast = ast; }

    /** The type defined by this type definition. */
    Type* type() const { return _type; }
    void setType(Type* type) { _type = type; }

    /** If this definition is a type alias, then this points to the target. */
    Type* aliasTarget() const { return _aliasTarget; }
    void setAliasTarget(Type* aliasTarget) { _aliasTarget = aliasTarget; }

    /** List of all members of this type. */
    DefnList& members() { return _members; }
    const DefnArray members() const { return _members; }

    /** Scope containing all of the members of this type. */
    SymbolTable* memberScope() const { return _memberScope.get(); }

    /** List of definitions this one extends. These can be specializations. */
    MemberList& extends() { return _extends; }
    const MemberArray extends() const { return _extends; }

    /** List of definitions this one implements. These can be specializations. */
    MemberList& implements() { return _implements; }
    const MemberArray implements() const { return _implements; }

    /** If this type is an intrinsic type, here is the information for it. */
    IntrinsicType intrinsic() const { return _intrinsic; }
    void setIntrinsic(IntrinsicType i) { _intrinsic = i; }

    /** Flag indicating whether extends/inherits has been resolved yet. */
    bool baseTypesResolved() const { return _baseTypesResolved; }
    void setBaseTypesResolved(bool value) { _baseTypesResolved = value; }

    /** Dynamic casting support. */
    static bool classof(const TypeDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::TYPE; }

  private:
    Type* _type;
    Type* _aliasTarget = nullptr;
    const ast::TypeDefn* _ast = nullptr;
    DefnList _members;
    MemberList _extends;
    MemberList _implements;
    std::unique_ptr<SymbolTable> _memberScope;
    IntrinsicType _intrinsic = IntrinsicType::NONE;
    bool _baseTypesResolved = false;


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
    {}

    /** AST for this parameter definition. */
    const ast::TypeParameter* ast() const { return _ast; }
    void setAst(const ast::TypeParameter* ast) { _ast = ast; }

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
    const ast::TypeParameter* _ast = nullptr;
    Type* _valueType;
    TypeVar* _typeVar;
    Type* _defaultType;
    int32_t _index;
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

    /** Abstract syntax tree for this member. */
    const ast::ValueDefn* ast() const { return _ast; }
    void setAst(const ast::ValueDefn* ast) { _ast = ast; }

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
    const ast::ValueDefn* _ast = nullptr;
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

    /** AST for this parameter definition. */
    const ast::Parameter* ast() const { return _ast; }
    void setAst(const ast::Parameter* ast) { _ast = ast; }

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
    const ast::Parameter* _ast = nullptr;
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
      , _variadic(false)
      , _methodIndex(0)
    {}

    ~FunctionDefn();

    /** Type of this function. */
    FunctionType* type() const { return _type; }
    void setType(FunctionType* type) { _type = type; }

    /** AST for this function. */
    const ast::Function* ast() const { return _ast; }
    void setAst(const ast::Function* ast) { _ast = ast; }

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
    DefnList& localDefns() { return _localDefns; }
    const DefnArray localDefns() const { return _localDefns; }

    /** True if this function is a constructor. */
    bool isConstructor() const { return _constructor; }
    void setConstructor(bool ctor) { _constructor = ctor; }

    /** True if this function is a requirement. */
    bool isRequirement() const { return _requirement; }
    void setRequirement(bool req) { _requirement = req; }

    /** True if this is a native function. */
    bool isNative() const { return _native; }
    void setNative(bool native) { _native = native; }

    /** True if the last parameter is a 'rest' param. */
    bool isVariadic() const { return _variadic; }
    void setVariadic(bool variadic) { _variadic = variadic; }

    /** If this type is an intrinsic type, here is the information for it. */
    // intrinsic::IntrinsicFunction* intrinsic() const { return _intrinsic; }
    // void setIntrinsic(intrinsic::IntrinsicFunction* i) { _intrinsic = i; }

    /** Method index for this function in a dispatch table. */
    int32_t methodIndex() const { return _methodIndex; }
    void setMethodIndex(int32_t index) { _methodIndex = index; }

    /** Dynamic casting support. */
    static bool classof(const FunctionDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::FUNCTION; }

  private:
    FunctionType* _type;
    const ast::Function* _ast;
    std::vector<ParameterDefn*> _params;
    std::unique_ptr<SymbolTable> _paramScope;
    DefnList _localDefns;
    // intrinsic::IntrinsicFunction* _intrinsic;
    Type* _selfType;
    Expr* _body;
    bool _constructor;
    bool _requirement;
    bool _native;
    bool _variadic;
    int32_t _methodIndex;
    //evaluable : bool = 12;        # If true, function can be evaluated at compile time.
  };

  /** A specialized generic definition. */
  class SpecializedDefn : public Member {
  public:
    SpecializedDefn(
        Member* generic,
        const llvm::ArrayRef<const Type*>& typeArgs,
        const llvm::ArrayRef<TypeParameter*>& typeParams)
      : Member(Kind::SPECIALIZED, generic->name())
      , _generic(generic)
      , _typeArgs(typeArgs.begin(), typeArgs.end())
      , _typeParams(typeParams)
    {
    }

    /** The generic type that this is a specialiation of. Note that this could be
        a member of a template as well as the template itself. */
    Member* generic() const { return _generic; }

    /** If this generic definition is a type then here's the corresponding type object. */
    SpecializedType* type() const { return _type; }
    void setType(SpecializedType* type) { _type = type; }

    /** The array of type arguments for this type. */
    const llvm::ArrayRef<const Type*> typeArgs() const { return _typeArgs; }

    /** The array of type parameters that the args are mapped to. */
    const llvm::ArrayRef<TypeParameter*> typeParams() const { return _typeParams; }

    /** Dynamic casting support. */
    static bool classof(const SpecializedDefn* m) { return true; }
    static bool classof(const Member* m) { return m->kind == Kind::SPECIALIZED; }

  private:
    Member* _generic;
    SpecializedType* _type = nullptr;
    llvm::SmallVector<const Type*, 2> _typeArgs;
    llvm::ArrayRef<TypeParameter*> _typeParams;
  };

  inline Defn* unwrapSpecialization(Member* m) {
    if (!m) {
      return nullptr;
    }
    while (m->kind == Defn::Kind::SPECIALIZED) {
      m = static_cast<SpecializedDefn*>(m)->generic();
    }
    return llvm::dyn_cast<Defn>(m);
  }
}

#endif
