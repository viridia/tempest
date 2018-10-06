#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/names/createnameref.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/support/allocator.hpp"

namespace tempest::sema::names {
  using namespace tempest::sema::graph;
  using tempest::source::Location;
  using tempest::support::BumpPtrAllocator;
  using tempest::error::diag;

  Expr* createNameRef(
      support::BumpPtrAllocator& alloc,
      const Location& loc,
      const NameLookupResultRef& result,
      Defn* subject,
      Expr* stem,
      bool preferPrivate,
      bool useADL) {
    NameLookupResult vars;
    NameLookupResult functions;
    NameLookupResult types;
    NameLookupResult privateMembers;
    NameLookupResult protectedMembers;

    for (auto member : result) {
      auto m = unwrapSpecialization(member);
      if (!isVisibleMember(subject, m)) {
        if (m->visibility() == PRIVATE) {
          privateMembers.push_back(m);
        } else if (m->visibility() == PROTECTED) {
          protectedMembers.push_back(m);
        }
      }

      if (m->kind == Member::Kind::TYPE || m->kind == Member::Kind::TYPE_PARAM) {
        types.push_back(member);
      } else if (m->kind == Member::Kind::FUNCTION) {
        functions.push_back(member);
      } else if (m->kind == Member::Kind::CONST_DEF
          || m->kind == Member::Kind::LET_DEF
          || m->kind == Member::Kind::ENUM_VAL
          || m->kind == Member::Kind::FUNCTION_PARAM
          || m->kind == Member::Kind::TUPLE_MEMBER) {
        vars.push_back(member);
      } else {
        assert(false && "Other member types not allowed here.");
      }
    }

    if (privateMembers.size() > 0) {
      auto p = unwrapSpecialization(privateMembers[0]);
      diag.error(loc) << "Cannot access private member '" << p->name() << "'.";
      diag.info(p->location()) << "Defined here.";
    } else if (protectedMembers.size() > 0) {
      auto p = unwrapSpecialization(protectedMembers[0]);
      diag.error(loc) << "Cannot access protected member '" << p->name() << "'.";
      diag.info(p->location()) << "Defined here.";
    } else {
      auto m = unwrapSpecialization(result[0]);
      if (vars.size() > 0) {
        if (functions.size() > 0 || types.size() > 0) {
          diag.error(loc) << "Conflicting definitions for '" << m->name() << "'.";
        } else if (vars.size() > 1) {
          diag.error(loc) << "Reference to '" << m->name() << "' is ambiguous.";
        } else {
          auto vd = static_cast<Defn*>(result[0]);
          if (stem && stem->kind == Expr::Kind::SELF && !vd->isMember()) {
            stem = nullptr;
          }
          return new (alloc) DefnRef(Expr::Kind::VAR_REF, loc, vd, stem);
        }
      } else if (types.size() > 0) {
        if (functions.size() > 0) {
          diag.error(loc) << "Conflicting definitions for '" << m->name() << "'.";
        } else {
          auto tref = new (alloc) MemberListExpr(
              Expr::Kind::TYPE_REF_OVERLOAD, loc, result[0]->name(), alloc.copyOf(types));
          tref->stem = stem;
          return tref;
        }
      } else {
        auto mref = new (alloc) MemberListExpr(
            Expr::Kind::FUNCTION_REF_OVERLOAD, loc, result[0]->name(), alloc.copyOf(functions));
        mref->useADL = useADL;
        mref->stem = stem;
        return mref;
      }
    }
    return nullptr;
  }
}
