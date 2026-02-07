#ifndef MissionTemplate_h
#define MissionTemplate_h

#include "Mission.h"

class MissionTemplate;
class MissionAlias;
class MissionCallsign;
class MissionEvent;

class MissionTemplate : public Mission
{
  public:
    MissionTemplate(int id, std::string_view filename = {}, std::string_view path = {});
    ~MissionTemplate() override;

    virtual bool Load(std::string_view filename = {}, std::string_view path = {});

    // accessors/mutators:
    virtual MissionElement* FindElement(std::string_view name);
    void AddElement(MissionElement* elem) override;
    virtual bool MapElement(MissionElement* elem);
    virtual std::string MapShip(std::string _name);
    virtual CombatGroup* GetPlayerSquadron() const { return player_squadron; }
    virtual void SetPlayerSquadron(CombatGroup* ps) { player_squadron = ps; }
    virtual std::string MapCallsign(std::string_view name, int iff);
    virtual bool MapEvent(MissionEvent* event);

  protected:
    CombatGroup* FindCombatGroup(int iff, const ShipDesign* dsn);
    void ParseAlias(TermStruct* _val);
    void ParseCallsign(TermStruct* val);
    bool ParseOptional(TermStruct* _val);
    void CheckObjectives();

    List<MissionAlias> aliases;
    List<MissionCallsign> callsigns;
    CombatGroup* player_squadron;
};

class MissionAlias
{
  friend class MissionTemplate;

  public:
    MissionAlias()
      : elem(nullptr) {}

    MissionAlias(std::string_view n, MissionElement* e)
      : name(n),
        elem(e) {}

    virtual ~MissionAlias() = default;

    int operator ==(const MissionAlias& a) const { return name == a.name; }

    std::string Name() const { return name; }
    MissionElement* Element() const { return elem; }

    void SetName(const char* n) { name = n; }
    void SetElement(MissionElement* e) { elem = e; }

  protected:
    std::string name;
    MissionElement* elem;
};

class MissionCallsign
{
  friend class MissionTemplate;

  public:
    MissionCallsign() = default;

    MissionCallsign(std::string_view c, std::string_view n)
      : call(c),
        name(n) {}

    virtual ~MissionCallsign() = default;

    int operator ==(const MissionCallsign& a) const { return call == a.call; }

    std::string Callsign() const { return call; }
    std::string Name() const { return name; }

    void SetCallsign(const char* c) { call = c; }
    void SetName(const char* n) { name = n; }

  protected:
    std::string call;
    std::string name;
};

#endif MissionTemplate_h
