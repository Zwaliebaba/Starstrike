#pragma once

#include "worldobject.h"

class Unit;
class InsertionSquad;
class ShapeStatic;
class TeamControls;

// ****************************************************************************
//  Class Entity
// ****************************************************************************

class Entity : public WorldObject
{
  public:
    enum
    {
      TypeInvalid = 0,
      // Remember to update Entity::GetTypeName and
      TypeLaserTroop,
      // Entity::NewEntity if you update this table
      TypeEngineer,
      TypeVirii,
      TypeInsertionSquadie,
      TypeEgg,
      TypeSporeGenerator,
      TypeLander,
      TypeTripod,
      TypeCentipede,
      TypeSpaceInvader,
      TypeSpider,
      TypeDarwinian,
      TypeOfficer,
      TypeArmyAnt,
      TypeArmour,
      TypeSoulDestroyer,
      TypeTriffidEgg,
      TypeAI,
      NumEntityTypes
    };

    enum
    {
      StatHealth = 0,
      StatSpeed,
      StatRate,
      NumStats
    };

    int m_formationIndex; // Our offset within the unit (NOT our index in the array)
    int m_buildingId; // Which building created us

    LegacyVector3 m_spawnPoint; // Where I was created
    float m_roamRange; // How far can I roam

    unsigned char m_stats[NumStats];
    bool m_dead; // Used to fade out dead creatures
    bool m_justFired;
    float m_reloading; // Time left to reload
    float m_inWater; // -1 = no, otherwise = time in water

    LegacyVector3 m_front;
    LegacyVector3 m_angVel;

    ShapeStatic* m_shape; // Might be NULL
    LegacyVector3 m_centerPos;
    float m_radius; // Can be Zero, which means its a sprite

    bool m_renderDamaged;

    int m_routeId;
    int m_routeWayPointId;
    float m_routeTriggerDistance; // distance the unit must be from the route node to move on to the next

    Entity();
    ~Entity() override;

    void SetType(unsigned char _type); // Loads default stats from blueprint

    void Render(float predictionTime) override;

    virtual void Begin();
    virtual bool Advance(Unit* _unit);
    virtual bool AdvanceDead(Unit* _unit);
    virtual void AdvanceInAir(Unit* _unit);
    virtual void AdvanceInWater(Unit* _unit);

    virtual void ChangeHealth(int amount);
    virtual void Attack(const LegacyVector3& pos);

    virtual bool IsInView();

    virtual void SetWaypoint(LegacyVector3 _waypoint);

    virtual LegacyVector3 PushFromObstructions(const LegacyVector3& pos, bool killem = true);
    virtual LegacyVector3 PushFromCliffs(const LegacyVector3& pos, const LegacyVector3& oldPos);
    virtual LegacyVector3 PushFromEachOther(const LegacyVector3& _pos);
    virtual int EnterTeleports(int _requiredId = -1); // Searches for valid nearby teleport entrances. Returns which one entered

    virtual void DirectControl(const TeamControls& _teamControls);

    static void BeginRenderShadow();
    static void RenderShadow(const LegacyVector3& _pos, float _size);
    static void EndRenderShadow();

    static const char* GetTypeName(int _troopType);
    static int GetTypeId(const char* _typeName);
    static Entity* NewEntity(int _troopType);

    static const char* GetTypeNameTranslated(int _troopType);

    bool RayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir);

    virtual LegacyVector3 GetCameraFocusPoint(); // used in unit tracking to determine the position the camera should look at
    void FollowRoute();
};

// ****************************************************************************
//  Class EntityBlueprint
// ****************************************************************************

class EntityBlueprint
{
  protected:
    static char* m_names[Entity::NumEntityTypes];
    static float m_stats[Entity::NumEntityTypes][Entity::NumStats];

  public:
    static void Initialise();

    static char* GetName(unsigned char _type);
    static float GetStat(unsigned char _type, int _stat);
};
