#pragma once

#include "Director.h"

class Sim;
class Ship;
class ShipAI;
class Instruction;
class Hangar;
class Element;
class FlightPlanner;

class CarrierAI : public Director
{
  public:
    CarrierAI(Ship* s, int level);
    ~CarrierAI() override = default;

    void ExecFrame(double seconds) override;

  protected:
    virtual bool CheckPatrolCoverage();
    virtual bool CheckHostileElements();

    virtual bool CreateStrike(Element* elem);

    virtual Element* CreatePackage(int squad, int size, int code, std::string_view target = {},
                                   std::string_view loadname = {});
    virtual bool LaunchElement(Element* elem);

    Sim* m_sim = {};
    Ship* m_ship = {};
    Hangar* m_hangar = {};
    std::unique_ptr<FlightPlanner> m_flightPlanner;
    int exec_time;
    int hold_time;
    int ai_level;

    Element* patrol_elem[4];
};
