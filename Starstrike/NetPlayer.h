#ifndef NetPlayer_h
#define NetPlayer_h

#include "Director.h"
#include "Geometry.h"
#include "SimObject.h"


class Sim;
class Ship;
class NetMsg;
class NetObjLoc;
class NetObjHyper;
class NetObjTarget;
class NetObjEmcon;
class NetSysDamage;
class NetSysStatus;
class NetWepTrigger;
class NetWepRelease;
class NetWepDestroy;
class NetCommMsg;

class NetPlayer : public Director, public SimObserver
{
  public:
    
    NetPlayer(DWORD nid)
      : netid(nid),
        objid(0),
        ship(nullptr),
        iff(0),
        bleed_time(0) {}

    ~NetPlayer() override;

    int operator ==(const NetPlayer& p) const { return netid == p.netid; }

    DWORD GetNetID() const { return netid; }
    DWORD GetObjID() const { return objid; }
    void SetObjID(DWORD o) { objid = o; }

    int GetIFF() const { return iff; }
    Ship* GetShip() const { return ship; }
    void SetShip(Ship* s);

    std::string Name() const { return name; }
    void SetName(std::string_view n) { name = n; }
    std::string SerialNumber() const { return serno; }
    void SetSerialNumber(std::string_view n) { serno = n; }

    void ExecFrame(double seconds) override;

    bool DoObjLoc(NetObjLoc* obj_loc);
    bool DoObjHyper(NetObjHyper* obj_hyper);
    bool DoObjTarget(NetObjTarget* obj_target);
    bool DoObjEmcon(NetObjEmcon* obj_emcon);
    bool DoWepTrigger(NetWepTrigger* trigger);
    bool DoWepRelease(NetWepRelease* release);
    bool DoCommMessage(NetCommMsg* comm_msg);
    bool DoSysDamage(NetSysDamage* sys_damage);
    bool DoSysStatus(NetSysStatus* sys_status);

    int Type() const override { return 2; }

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

  protected:
    DWORD netid;
    DWORD objid;
    std::string name;
    std::string serno;
    Ship* ship;
    int iff;

    Point loc_error;
    double bleed_time;
};

#endif NetPlayer_h
