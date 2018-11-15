#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/pass/findoverrides.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/transform/mapenv.hpp"
#include <assert.h>

namespace tempest::sema::pass {
  using llvm::StringRef;
  using tempest::error::diag;
  using namespace llvm;
  using namespace tempest::sema::graph;

  // Compare two functions and return true if their signatures are the same.
  // Note that this does *not* include selfType.
  bool equalSignatures(
      FunctionDefn* fa, const TypeArray& faArgs, FunctionDefn* fb, const TypeArray& fbArgs) {
    if (fa->isVariadic() != fb->isVariadic()) {
      return false;
    }
    if (fa->params().size() != fb->params().size()) {
      return false;
    }
    if (fa->isMutableSelf() != fb->isMutableSelf()) {
      return false;
    }
    Env faEnv;
    faEnv.params = fa->allTypeParams();
    faEnv.args = faArgs;
    Env fbEnv;
    fbEnv.params = fb->allTypeParams();
    fbEnv.args = fbArgs;

    for (size_t i = 0; i < fa->params().size(); i += 1) {
      if (!convert::isEqual(
          fa->params()[i]->type(), 0, faEnv,
          fb->params()[i]->type(), 0, fbEnv)) {
        return false;
      }
    }

    return convert::isEqual(
        fa->type()->returnType, 0, faEnv, fb->type()->returnType, 0, fbEnv);
  }

  bool canOverride(
      FunctionDefn* m, const TypeArray& mArgs, FunctionDefn* im, const TypeArray& imArgs) {
    if (m->isVariadic() != im->isVariadic()) {
      return false;
    }
    if (m->params().size() != im->params().size()) {
      return false;
    }
    if (m->isMutableSelf() && !im->isMutableSelf()) {
      return false;
    }
    Env mEnv;
    mEnv.params = m->allTypeParams();
    mEnv.args = mArgs;
    Env imEnv;
    imEnv.params = im->allTypeParams();
    imEnv.args = imArgs;

    for (size_t i = 0; i < m->params().size(); i += 1) {
      if (!convert::isEqual(
          m->params()[i]->type(), 0, mEnv,
          im->params()[i]->type(), 0, imEnv)) {
        return false;
      }
    }

    // Return type covariance.
    return convert::isEqualOrNarrower(
        m->type()->returnType, mEnv, im->type()->returnType, imEnv);
  }

  void FindOverridesPass::run() {
    while (_sourcesProcessed < _cu.sourceModules().size()) {
      process(_cu.sourceModules()[_sourcesProcessed++]);
    }
    while (_importSourcesProcessed < _cu.importSourceModules().size()) {
      process(_cu.importSourceModules()[_importSourcesProcessed++]);
    }
  }

  void FindOverridesPass::process(Module* mod) {
    // diag.info() << "Resolving imports: " << mod->name();
    _alloc = &mod->semaAlloc();
    visitMembers(mod->members(), nullptr);
    visitList(mod->members());
  }

  void FindOverridesPass::visitList(DefnArray members) {
    for (auto defn : members) {
      if (auto td = dyn_cast<TypeDefn>(defn)) {
        visitTypeDefn(static_cast<TypeDefn*>(defn));
      }
    }
  }

  void FindOverridesPass::visitTypeDefn(TypeDefn* td) {
    if (auto udt = dyn_cast<UserDefinedType>(td->type())) {
      visitCompositeDefn(td);
    }
  }

  void FindOverridesPass::visitCompositeDefn(TypeDefn* td) {
    if (td->overridesFound()) {
      return;
    }

    // Make sure base types are analyzed first.
    for (auto base : td->extends()) {
      visitCompositeDefn(cast<TypeDefn>(unwrapSpecialization(base)));
    }
    for (auto base : td->implements()) {
      visitCompositeDefn(cast<TypeDefn>(unwrapSpecialization(base)));
    }

    // Populate the method table with a copy of the methods from the superclass.
    if (td->extends().size() > 0) {
      ArrayRef<const Type*> baseTypeArgs;
      auto baseDefn = cast<TypeDefn>(unwrapSpecialization(td->extends()[0], baseTypeArgs));
      transform::MapEnvTransform mapTypeArgs(
          _cu.types(), _cu.spec(), baseDefn->allTypeParams(), baseTypeArgs);
      for (auto& entry : baseDefn->methods()) {
        td->methods().push_back({ entry.method, mapTypeArgs.transformArray(entry.typeArgs) });
      }
    }

    // Create interface tables for each implemented interface. Fill with null values.
    if (td->type()->kind == Type::Kind::CLASS || td->type()->kind == Type::Kind::STRUCT) {
      for (auto ifc : td->implements()) {
        ArrayRef<const Type*> baseTypeArgs;
        auto baseDefn = cast<TypeDefn>(unwrapSpecialization(ifc, baseTypeArgs));
        td->interfaceMethods().push_back(MethodTable());
        auto& table = td->interfaceMethods().back();
        table.resize(baseDefn->methods().size(), { nullptr, {} });
      }
    }

    visitMembers(td->members(), td);
    appendNewMethods(td);
    td->setOverridesFound(true);
  }

  void FindOverridesPass::visitMembers(const DefnList& members, TypeDefn* td) {

    // Build map of all inherited methods by name.
    InheritedMethodMap membersByName;
    for (auto m : members) {
      membersByName[m->name()].push_back({ DEFINED, 0, m, {} });
    }

    InheritedMethodMap inheritedMembersByName;
    if (td) {
      for (auto base : td->extends()) {
        ArrayRef<const Type*> typeArgs;
        auto baseDefn = unwrapSpecialization(base, typeArgs);
        size_t index = 0;
        for (auto member : cast<TypeDefn>(baseDefn)->members()) {
          inheritedMembersByName[member->name()].push_back({
            INHERIT_EXTENDS, index, member, typeArgs
          });
        }
      }

      for (auto base : td->implements()) {
        ArrayRef<const Type*> typeArgs;
        auto baseDefn = unwrapSpecialization(base, typeArgs);
        size_t index = 0;
        for (auto member : cast<TypeDefn>(baseDefn)->members()) {
          inheritedMembersByName[member->name()].push_back({
            INHERIT_IMPLEMENTS, index, member, typeArgs
          });
        }
      }
    }

    // For each unique name
    for (auto& memberEntry : membersByName) {
      // Split into groups by metatype
      SmallVector<TypeDefn*, 4> memberTypes;
      SmallVector<FunctionDefn*, 4> memberFuncs;
      SmallVector<ValueDefn*, 4> memberVars;

      for (auto& im : memberEntry.second) {
        if (auto td = dyn_cast<TypeDefn>(im.memberDefn)) {
          memberTypes.push_back(td);
        } else if (auto fd = dyn_cast<FunctionDefn>(im.memberDefn)) {
          memberFuncs.push_back(fd);
        } else if (auto vd = dyn_cast<ValueDefn>(im.memberDefn)) {
          memberVars.push_back(vd);
        }
      }

      // Split inherited methods by metatype
      auto it = inheritedMembersByName.find(memberEntry.first());
      SmallVector<Inherited<TypeDefn>, 4> inheritedTypes;
      SmallVector<Inherited<FunctionDefn>, 4> inheritedFuncs;
      SmallVector<Inherited<ValueDefn>, 4> inheritedVars;
      if (it != inheritedMembersByName.end()) {
        for (auto& im : it->second) {
          if (auto td = dyn_cast<TypeDefn>(im.memberDefn)) {
            inheritedTypes.push_back({ im.itype, im.baseIndex, td, im.typeArgs });
          } else if (auto fd = dyn_cast<FunctionDefn>(im.memberDefn)) {
            inheritedFuncs.push_back({ im.itype, im.baseIndex, fd, im.typeArgs });
          } else if (auto vd = dyn_cast<ValueDefn>(im.memberDefn)) {
            inheritedVars.push_back({ im.itype, im.baseIndex, vd, im.typeArgs });
          }
        }
      }

      // Don't allow members of different metatypes with the same name
      bool conflict = false;
      bool inheritedConflict = false;
      bool multiDefinedVar = false;
      if (memberTypes.size() > 0) {
        if (memberFuncs.size() > 0 || memberVars.size() > 0) {
          conflict = true;
        } else if (inheritedFuncs.size() > 0 || inheritedVars.size() > 0) {
          inheritedConflict = true;
        }
      } else if (memberFuncs.size() > 0) {
        if (memberVars.size() > 0) {
          conflict = true;
        } else if (inheritedTypes.size() > 0 || inheritedVars.size() > 0) {
          inheritedConflict = true;
        }
      } else if (memberVars.size() > 0) {
        if (inheritedTypes.size() > 0 || inheritedFuncs.size() > 0) {
          inheritedConflict = true;
        } else if (memberVars.size() > 1) {
          multiDefinedVar = true;
        }
      }

      if (conflict) {
        bool first = true;
        for (auto& m : memberEntry.second) {
          if (first) {
            diag.error(m.memberDefn) <<
                "Conflicting definitions for '" << memberEntry.first() << "'.";
          } else {
            diag.info(m.memberDefn->location()) << "also defined here.";
          }
        }
      } else if (inheritedConflict) {
        diag.error(memberEntry.second.front().memberDefn->location()) <<
            "Definition of '" << memberEntry.first() << "' conflicts with inherited definition.";
        for (auto& m : it->second) {
          diag.info(m.memberDefn->location()) << "defined here.";
        }
      } else if (multiDefinedVar) {
        bool first = true;
        for (auto& m : memberEntry.second) {
          if (first) {
            diag.error(m.memberDefn) <<
                "Multiple definitions for not allowed for '" << memberEntry.first() << "'.";
          } else {
            diag.info(m.memberDefn->location()) << "also defined here.";
          }
        }
      } else if (memberFuncs.size() > 0) {
        findSameNamedOverrides(td, memberFuncs, inheritedFuncs);
      } else if (memberVars.size() > 0) {
        // TODO: Implement
      } else if (memberTypes.size() > 0) {
        // TODO: Implement
      }
    }

    // For each inherited function
    for (auto& memberEntry : inheritedMembersByName) {
      auto it = inheritedMembersByName.find(memberEntry.first());
      SmallVector<Inherited<FunctionDefn>, 4> inheritedFuncs;
      if (it != inheritedMembersByName.end()) {
        for (auto& im : it->second) {
          if (auto fd = dyn_cast<FunctionDefn>(im.memberDefn)) {
            inheritedFuncs.push_back({ im.itype, im.baseIndex, fd, im.typeArgs });
          }
        }
      }

      fillEmptySlots(td, inheritedFuncs);
    }
  }

  void FindOverridesPass::findSameNamedOverrides(
      TypeDefn* td,
      SmallVectorImpl<FunctionDefn*>& methods,
      SmallVectorImpl<Inherited<FunctionDefn>>& inherited) {
    auto enclosingKind = td ? td->type()->kind : Type::Kind::VOID;

    // Remove all static members from consideration
    // NOTE: 'methods' and 'inherited' are the subset of methods that have the same name.
    methods.erase(
        std::remove_if(methods.begin(), methods.end(), [](auto m) {
          return m->isStatic()
              || m->isConstructor();
        }), methods.end());
    inherited.erase(
        std::remove_if(inherited.begin(), inherited.end(), [](auto& m) {
          return m.memberDefn->isStatic()
              || m.memberDefn->isConstructor();
        }), inherited.end());

    // Don't allow methods of identical types
    if (findEqualSignatures(methods)) {
      return;
    }

    for (auto fd : methods) {
      if (fd->visibility() != Visibility::PRIVATE) {
        // Means we haven't set the method index yet.
        fd->setMethodIndex(-1);
      }

      if (fd->isOverride()) {
        if (enclosingKind != Type::Kind::CLASS) {
          diag.error(fd->location()) << "Only class methods can be declared as 'override'.";
          break;
        } else if (fd->isAbstract()) {
          diag.error(fd->location()) << "Method overrides cannot be abstract.";
          break;
        } else if (fd->visibility() == Visibility::PRIVATE) {
          diag.error(fd->location()) << "Method overrides cannot be private.";
          break;
        } else if (inherited.empty()) {
          diag.error(fd->location()) << "Method '" << fd->name()
              << "' declared as 'override' but there is no inherited method of that name.";
          break;
        }
      }

      SmallVector<Inherited<FunctionDefn>, 4> fromExtends;
      SmallVector<Inherited<FunctionDefn>, 4> fromImplements;

      for (auto& im : inherited) {
        if (canOverride(fd, {}, im.memberDefn, im.typeArgs)) {
          if (im.itype == INHERIT_EXTENDS) {
            fromExtends.push_back(im);
          } else if (im.itype == INHERIT_IMPLEMENTS) {
            fromImplements.push_back(im);
          }
        }
      }

      // Handle overrides of methods declared in 'extends'
      if (fromExtends.size() > 0) {
        // There should be at most one.
        if (fromExtends.size() > 1) {
          diag.error(fd->location()) << "Ambiguous override for method '" << fd->name()
              << "': more than one matching method in base type.";
          for (auto im : fromExtends) {
            diag.info(im.memberDefn->location()) << "Defined here.";
          }
          break;
        }

        // Rules for overriding
        auto inheritedMethod = fromExtends[0].memberDefn;
        auto extendsType = dyn_cast_or_null<TypeDefn>(inheritedMethod->definedIn());
        auto extendsKind = extendsType ? extendsType->type()->kind : Type::Kind::VOID;
        if (inheritedMethod->isFinal()) {
          diag.error(fd->location()) << "Method '" << fd->name()
              << "' cannot override inherited method declared as 'final'.";
          diag.info(inheritedMethod->location()) << "Defined here.";
        } else if (extendsType && extendsType->isFinal()) {
          diag.error(fd->location()) << "Method '" << fd->name()
              << "' cannot override inherited method declared in final class.";
          diag.info(inheritedMethod->location()) << "Defined here.";
        } else if (inheritedMethod->visibility() == Visibility::PRIVATE) {
          diag.error(fd->location()) << "Method '" << fd->name()
              << "' overrides inherited method which is not visible.";
          diag.info(inheritedMethod->location()) << "Defined here.";
        } else if (inheritedMethod->visibility() == Visibility::PUBLIC &&
            fd->visibility() != Visibility::PUBLIC) {
          diag.error(fd->location()) << "Method '" << fd->name()
              << "' cannot restrict visibility of inherited method.";
          diag.info(inheritedMethod->location()) << "Defined here.";
        } else if (extendsKind == Type::Kind::CLASS && !fd->isOverride()) {
          diag.error(fd->location()) << "Method '" << fd->name()
              << "' that overrides an inherited method must be declared with 'override'.";
          diag.info(inheritedMethod->location()) << "Inherited from here.";
        } else {
          assert(inheritedMethod->methodIndex() >= 0);
          fd->setMethodIndex(inheritedMethod->methodIndex());
          MethodTableEntry& mte = td->methods()[inheritedMethod->methodIndex()];
          mte.method = fd;
          mte.typeArgs = ArrayRef<const Type*>();
        }
      } else if (fd->isOverride()) {
        diag.error(fd->location()) << "Method '" << fd->name()
            << "' declared as 'override' but does not match the type signature of any "
            << "inherited method.";
      }

      // Handle overrides of methods declared in 'implements'
      if (fromImplements.size() > 0) {
        auto inheritedMethod = fromImplements[0].memberDefn;
        if (fd->visibility() == Visibility::PRIVATE) {
          diag.error(fd->location()) <<
              "Trait or interface method cannot be implemented by a private method.";
          diag.info(inheritedMethod->location()) << "Inherited from here.";
          break;
        }
      }

      for (auto& im : fromImplements) {
        assert(im.baseIndex >= 0);
        assert(im.baseIndex < td->interfaceMethods().size());
        auto& methods = td->interfaceMethods()[im.baseIndex];
        assert(im.memberDefn->methodIndex() >= 0);
        assert(im.memberDefn->methodIndex() < (int32_t) methods.size());
        auto& entry = methods[im.memberDefn->methodIndex()];
        entry.method = fd;
        entry.typeArgs = ArrayRef<const Type*>();
      }
    }
  }

  void FindOverridesPass::fillEmptySlots(
      TypeDefn* td,
      SmallVectorImpl<Inherited<FunctionDefn>>& inherited) {

    inherited.erase(
        std::remove_if(inherited.begin(), inherited.end(), [](auto& m) {
          return m.memberDefn->isStatic()
              || m.memberDefn->isConstructor();
        }), inherited.end());

    // Ummm...error here: we forgot to handle interface slots for abstract base classes.
    // For each method declared in an 'implements' type

    // Now fill in any remaining empty slots
    auto enclosingKind = td ? td->type()->kind : Type::Kind::VOID;
    for (auto& im : inherited) {
      if (im.itype == INHERIT_EXTENDS) {
        if (enclosingKind == Type::Kind::CLASS &&
            !td->isAbstract() &&
            im.memberDefn->isAbstract()) {
          assert(im.memberDefn->methodIndex() >= 0);
          auto& entry = td->methods()[im.memberDefn->methodIndex()];
          if (entry.method == nullptr || entry.method->isAbstract()) {
            diag.error(td->location()) <<
                "Non-abstract class '" << td->name() <<
                "' lacks concrete implementation for method '" <<
                im.memberDefn->definedIn()->name() << "'." <<
                im.memberDefn->name() << "'.";
          }
        }
      } else if (im.itype == INHERIT_IMPLEMENTS) {
        assert(im.baseIndex >= 0);
        assert(im.baseIndex < td->interfaceMethods().size());
        auto& methods = td->interfaceMethods()[im.baseIndex];
        assert(im.memberDefn->methodIndex() >= 0);
        assert(im.memberDefn->methodIndex() < (int32_t) methods.size());
        auto& entry = methods[im.memberDefn->methodIndex()];
        // If the interface method slot is empty, try finding an inherited method that
        // will work.
        if (entry.method == nullptr) {
          for (auto& md : inherited) {
            if (md.itype == INHERIT_EXTENDS &&
                !md.memberDefn->isAbstract() &&
                md.memberDefn->visibility() != Visibility::PRIVATE &&
                canOverride(md.memberDefn, md.typeArgs, im.memberDefn, im.typeArgs)) {
              entry.method = md.memberDefn;
              entry.typeArgs = md.typeArgs;
              break;
            }
          }
        }

        if (entry.method == nullptr && !td->isAbstract()) {
          diag.error(td->location()) <<
              "Type '" << td->name() << "' declared as implementing type '" <<
              im.memberDefn->definedIn()->name() << "', but missing definition for: '" <<
              im.memberDefn->name() << "'.";
          diag.info(im.memberDefn->location()) << "Defined here.";
          NameLookupResult lookupResult;
          td->memberScope()->lookupName(im.memberDefn->name(), lookupResult);
          for (auto lr : lookupResult) {
            auto def = cast<Defn>(lr);
            if (def->kind != Member::Kind::FUNCTION) {
              diag.error(def->location()) << "Can't override: not a function";
              continue;
            }
            auto fd = cast<FunctionDefn>(def);
            Env mEnv;
            mEnv.params = fd->allTypeParams();
            Env imEnv;
            imEnv.params = im.memberDefn->allTypeParams();
            imEnv.args = im.typeArgs;

            if (fd->visibility() == Visibility::PRIVATE) {
              diag.info(fd->location()) << "Can't override: method is private.";
            } else if (fd->isStatic()) {
              diag.info(fd->location()) << "Can't override: method is static.";
            } else if (fd->isAbstract()) {
              diag.info(fd->location()) << "Can't override: method is abstract.";
            } else if (fd->isConstructor()) {
              diag.info(fd->location()) << "Can't override: constructor.";
            } else if (fd->isMutableSelf() && !im.memberDefn->isMutableSelf()) {
              diag.info(fd->location()) << "Can't override: method mutates self.";
            } else if (fd->isVariadic() && !im.memberDefn->isVariadic()) {
              diag.info(fd->location()) << "Can't override: mismatched variadic params.";
            } else if (!convert::isEqualOrNarrower(
                fd->type()->returnType, mEnv,
                im.memberDefn->type()->returnType, imEnv)) {
              diag.info(fd->location()) << "Can't override: mismatched return type: " <<
                  fd->type()->returnType;
            } else {
              // if (m->params().size() != im->params().size()) {
              //   return false;
              // }
              // TODO: Finish
              diag.error(def->location()) << "Unsuitable/implement param type report";
            }
          }
        }
      }
    }
  }

  void FindOverridesPass::appendNewMethods(TypeDefn* td) {
    if (!td->isFinal()) {
      SmallVector<const Type*, 4> defaultTypeArgs;
      for (auto param : td->allTypeParams()) {
        defaultTypeArgs.push_back(param->typeVar());
      }
      auto typeArgs = _alloc->copyOf(defaultTypeArgs);
      for (auto m : td->members()) {
        if (auto fd = dyn_cast<FunctionDefn>(m)) {
          if (!fd->isConstructor() &&
              !fd->isStatic() &&
              !fd->isFinal() &&
              !fd->isOverride() &&
              fd->visibility() != Visibility::PRIVATE
              && fd->methodIndex() < 0) {
            fd->setMethodIndex(td->methods().size());
            td->methods().push_back({ fd, typeArgs });
          }
        }
      }
    }
  }

  bool FindOverridesPass::findEqualSignatures(SmallVectorImpl<FunctionDefn*>& methods) {
    // Don't allow methods of identical types
    if (methods.size() > 1) {
      bool error = false;
      for (auto it = methods.begin(); it != methods.end(); ++it) {
        auto fa = *it;
        for (auto ot = it + 1; ot != methods.end(); ++ot) {
          if (equalSignatures(*it, {}, *ot, {})) {
            if (!error) {
              auto fb = *ot;
              error = true;
              diag.error(fb->location()) << "Method definition '" << fb->name()
                  << "' is indistinguishable from earlier method of the same name.";
              diag.info(fa->location()) << "Defined here.";
              return true;
            }
          }
        }
      }
    }
    return false;
  }
}
