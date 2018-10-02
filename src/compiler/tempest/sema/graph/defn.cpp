#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/type.hpp"
#include <assert.h>

namespace tempest::sema::graph {

  /** Return true if member is defined within the enclosing definition. */
  bool isDefinedIn(Member* subject, Member* enclosing) {
    while (subject) {
      if (subject == enclosing) {
        return true;
      } else if (auto defn = dyn_cast<Defn>(subject)) {
        subject = defn->definedIn();
      } else {
        break;
      }
    }
    return false;
  }

  /** Return true if member is defined within a base type of the enclosing definition. */
  bool isDefinedInBaseType(Member* subject, Member* enclosing) {
    while (subject) {
      if (subject == enclosing) {
        return true;
      } else if (auto enclosingType = dyn_cast<TypeDefn>(enclosing)) {
        for (auto extDef : enclosingType->extends()) {
          if (isDefinedInBaseType(subject, extDef)) {
            return true;
          }
        }
      }

      if (auto defn = dyn_cast<Defn>(subject)) {
        subject = defn->definedIn();
      } else {
        break;
      }
    }
    return false;
  }

  bool isVisibleMember(Member* subject, Member* target) {
    auto subjectDefn = unwrapSpecialization(subject);
    auto targetDefn = unwrapSpecialization(target);
    if (subjectDefn && targetDefn) {
      return isVisible(subjectDefn, targetDefn);
    }
    return false;
  }

  bool isVisible(Defn* subject, Defn* target) {
    // If the target has a visibility of PUBLIC then it's visible.
    if (target->visibility() == PUBLIC) {
      return true;
    }

    // If the subject is transitively contained within the immediate parent scope of target, then
    // target is visible.
    auto targetParent = target->definedIn();
    if (isDefinedIn(subject, targetParent)) {
      return true;
    }

    // If target is protected and subject is contained within a type that inherits from
    // target's parent, then target is visible.
    if (target->visibility() == PROTECTED && isDefinedInBaseType(subject, targetParent)) {
      return true;
    }

    // TODO: friends

    return false;
  }

  // void Defn::formatModifiers(std::ostream& out) const {
  //   if (_visibility == PRIVATE) {
  //     out << "private ";
  //   } else if (_visibility == PROTECTED) {
  //     out << "protected ";
  //   }

  //   if (_static) {
  //     out << "static ";
  //   }
  //   if (_final) {
  //     out << "final ";
  //   }
  //   if (_override) {
  //     out << "override ";
  //   }
  //   if (_abstract) {
  //     out << "abstract ";
  //   }
  //   if (_undef) {
  //     out << "undef ";
  //   }
  // }

  GenericDefn::~GenericDefn() {
    // for (auto tparam : _typeParams) {
    //   delete tparam;
    // }
    // for (auto iscope : _interceptScopes) {
    //   delete iscope.second;
    // }
  }

  TypeDefn::~TypeDefn() {
    for (Member* member : _members) {
      delete member;
    }
  }

  FunctionDefn::~FunctionDefn() {
    // for (ParameterDefn* param : _params) {
    //   delete param;
    // }
  }
}
