#ifndef CombatAssignment_h
#define CombatAssignment_h

#include "CombatGroup.h"

class CombatAssignment
{
  public:
    CombatAssignment(int t, CombatGroup* obj, CombatGroup* rsc = nullptr);
    ~CombatAssignment();

    int operator <(const CombatAssignment& a) const;

    // operations:
    void SetObjective(CombatGroup* o) { objective = o; }
    void SetResource(CombatGroup* r) { resource = r; }

    // accessors:
    [[nodiscard]] int Type() const { return type; }
    [[nodiscard]] CombatGroup* GetObjective() const { return objective; }
    [[nodiscard]] CombatGroup* GetResource() const { return resource; }

    [[nodiscard]] std::string GetDescription() const;
    bool IsActive() const { return resource != nullptr; }

  private:
    int type;
    CombatGroup* objective;
    CombatGroup* resource;
};

#endif CombatAssignment_h
