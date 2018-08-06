#include "tempest/ast/defn.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace tempest::sema::graph;

  void BuildGraphPass::run() {
    while (moreSources()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (moreImportSources()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
  }

  void BuildGraphPass::process(Module* mod) {
    // diag.info() << "Resolving imports: " << mod->name();
    _alloc = &mod->semaAlloc();
    auto modAst = static_cast<const ast::Module*>(mod->ast());
    createMembers(modAst->members, mod, mod->members(), mod->memberScope());
  }

  void BuildGraphPass::createMembers(
      const ast::NodeList& memberAsts,
      Member* parent,
      DefnList& memberList,
      SymbolTable* memberScope)
  {
    memberList.reserve(memberAsts.size());
    for (const ast::Node* node : memberAsts) {
      auto ast = static_cast<const ast::Defn*>(node);
      Defn* d = createDefn(node, parent);
      d->setVisibility(astVisibility(ast));
      d->setAbstract(ast->isAbstract());
      d->setFinal(ast->isFinal());
      d->setOverride(ast->isOverride());
      d->setUndef(ast->isUndef());
      d->setStatic(ast->isStatic());
      memberList.push_back(d);
      memberScope->addMember(d);

      if (node->kind == ast::Node::Kind::OBJECT_DEFN) {
        ValueDefn* singleton = new ValueDefn(
            Member::Kind::CONST_DEF, ast->location, ast->name, parent);
        singleton->setType(static_cast<TypeDefn*>(d)->type());
        singleton->setVisibility(astVisibility(ast));
        memberList.push_back(singleton);
        memberScope->addMember(singleton);
      }
    }
  }

  Defn* BuildGraphPass::createDefn(const ast::Node* node, Member* parent) {
    switch (node->kind) {
      case ast::Node::Kind::MEMBER_CONST:
      case ast::Node::Kind::MEMBER_VAR: {
        const ast::ValueDefn* ast = static_cast<const ast::ValueDefn*>(node);
        Member::Kind kind = node->kind == ast::Node::Kind::MEMBER_CONST ?
            Member::Kind::CONST_DEF : Member::Kind::LET_DEF;
        ValueDefn* vd = new ValueDefn(
            kind, ast->location, ast->name, parent);
        vd->setAst(ast);
        return vd;
      }

      case ast::Node::Kind::ENUM_VALUE: {
        const ast::ValueDefn* ast = static_cast<const ast::ValueDefn*>(node);
        ValueDefn* vd = new ValueDefn(
            Member::Kind::ENUM_VAL, ast->location, ast->name, parent);
        vd->setAst(ast);
        return vd;
      }

      case ast::Node::Kind::CLASS_DEFN:
      case ast::Node::Kind::STRUCT_DEFN:
      case ast::Node::Kind::INTERFACE_DEFN:
      case ast::Node::Kind::TRAIT_DEFN:
      case ast::Node::Kind::OBJECT_DEFN: {
        const ast::TypeDefn* ast = static_cast<const ast::TypeDefn*>(node);
        std::string typeName(ast->name.begin(), ast->name.end());
        if (node->kind == ast::Node::Kind::OBJECT_DEFN) {
          typeName.append("#Class");
        }
        TypeDefn* td = new TypeDefn(ast->location, typeName, parent);

        Type::Kind tk = Type::Kind::CLASS;
        if (node->kind == ast::Node::Kind::STRUCT_DEFN) {
          tk = Type::Kind::STRUCT;
        } else if (node->kind == ast::Node::Kind::INTERFACE_DEFN) {
          tk = Type::Kind::INTERFACE;
        } else if (node->kind == ast::Node::Kind::TRAIT_DEFN) {
          tk = Type::Kind::TRAIT;
        }
        UserDefinedType* cls = new (*_alloc) UserDefinedType(tk);
        td->setAst(ast);
        td->setType(cls);
        cls->setDefn(td);

        createTypeParamList(ast->typeParams, td, td->typeParams(), td->typeParamScope());
        createMembers(ast->members, td, td->members(), td->memberScope());

        // Make sure there are no name collisions between type params and member names.
        if (!td->typeParams().empty()) {
          for (auto member : td->members()) {
            NameLookupResult lookupResult;
            td->typeParamScope()->lookupName(member->name(), lookupResult);
            if (!lookupResult.empty()) {
              diag.error(member) << "Member name '"
                  << member->name() << "' shadows type parameter with the same name.";
            }
          }
        }
  //       decl.setRequiredMethodScope(StandardScope(decl, 'constraint'))
        return td;
      }
      case ast::Node::Kind::ENUM_DEFN: {
        const ast::TypeDefn* ast = static_cast<const ast::TypeDefn*>(node);
        TypeDefn* td = new TypeDefn(ast->location, ast->name, parent);

        UserDefinedType* cls = new (*_alloc) UserDefinedType(Type::Kind::ENUM);
        td->setAst(ast);
        td->setType(cls);
        cls->setDefn(td);

        createMembers(ast->members, td, td->members(), td->memberScope());
        return td;
      }
      case ast::Node::Kind::FUNCTION: {
        const ast::Function* ast = static_cast<const ast::Function*>(node);
        FunctionDefn* f = new FunctionDefn(ast->location, ast->name, parent);
        createParamList(ast->params, f, f->params(), f->paramScope());
        createTypeParamList(ast->typeParams, f, f->typeParams(), f->typeParamScope());
        f->setAst(ast);
        return f;
      }
      default:
        diag.fatal(node->location) << "Invalid member node type: " << node->kind;
        return nullptr;
    }
  }

  void BuildGraphPass::createParamList(
      const ast::NodeList& paramAsts,
      Member* parent,
      std::vector<ParameterDefn*>& paramList,
      SymbolTable* paramScope) {

    paramList.reserve(paramAsts.size());
    for (const ast::Node* node : paramAsts) {
      assert(node->kind == ast::Node::Kind::PARAMETER);
      const ast::Parameter* ast = static_cast<const ast::Parameter*>(node);
      ParameterDefn* param = new ParameterDefn(ast->location, ast->name, parent);
      param->setAst(ast);
      param->setKeywordOnly(ast->keywordOnly);
      param->setVariadic(ast->variadic);
      param->setSelfParam(ast->selfParam);
      param->setClassParam(ast->classParam);
      param->setExpansion(ast->expansion);

      paramList.push_back(param);
      paramScope->addMember(param);
    }
  }

  void BuildGraphPass::createTypeParamList(
      const ast::NodeList& paramAsts,
      Member* parent,
      std::vector<TypeParameter*>& paramList,
      SymbolTable* paramScope) {

    paramList.reserve(paramAsts.size());
    for (const ast::Node* node : paramAsts) {
      assert(node->kind == ast::Node::Kind::TYPE_PARAMETER);
      const ast::TypeParameter* ast = static_cast<const ast::TypeParameter*>(node);
      TypeParameter* param = new TypeParameter(
          ast->location, ast->name, parent);
      param->setAst(ast);
      param->setVariadic(ast->variadic);
      param->setTypeVar(new (*_alloc) TypeVar(param));
      paramList.push_back(param);
      paramScope->addMember(param);
    }
  }

  Visibility BuildGraphPass::astVisibility(const ast::Defn* d) {
    if (d->isPrivate()) {
      return PRIVATE;
    } else if (d->isProtected()) {
      return PROTECTED;
    } else {
      return PUBLIC;
    }
  }
}
