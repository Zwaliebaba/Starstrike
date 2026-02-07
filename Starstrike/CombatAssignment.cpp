#include "pch.h"
#include "CombatAssignment.h"
#include "CombatGroup.h"
#include "Mission.h"

CombatAssignment::CombatAssignment(int t, CombatGroup* obj, CombatGroup* rsc)
  : type(t),
    objective(obj),
    resource(rsc) {}

CombatAssignment::~CombatAssignment() {}

// This is used to sort assignments into a priority list.
// Higher priorities should come first in the list, so the
// sense of the operator is "backwards" from the usual.

int CombatAssignment::operator <(const CombatAssignment& a) const
{
  if (!objective)
    return 0;

  if (!a.objective)
    return 1;

  return objective->GetPlanValue() > a.objective->GetPlanValue();
}

std::string CombatAssignment::GetDescription() const
{
  std::string desc;

  if (!resource)
    desc = std::format("{} {}", Mission::RoleName(type), objective->Name());
  else
    desc = std::format("{} {} {}", resource->Name(), Mission::RoleName(type), objective->Name());

  return desc;
}
