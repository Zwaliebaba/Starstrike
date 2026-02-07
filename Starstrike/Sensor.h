#ifndef Sensor_h
#define Sensor_h

#include "List.h"
#include "SimObject.h"
#include "System.h"

class Shot;
class Contact;

class Sensor : public System, public SimObserver
{
  public:
    enum Mode
    {
      PAS,
      STD,
      ACM,
      GM,   // fighter modes
      PST,
      CST             // starship modes
    };

    Sensor();
    Sensor(const Sensor& rhs);
    ~Sensor() override;

    void ExecFrame(double seconds) override;
    virtual SimObject* LockTarget(int type = SimObject::SIM_SHIP, bool closest = false, bool hostile = false);
    virtual SimObject* LockTarget(SimObject* candidate);
    virtual bool IsTracking(SimObject* tgt);
    void DoEMCON(int emcon) override;

    virtual void ClearAllContacts();

    virtual Mode GetMode() const { return mode; }
    virtual void SetMode(Mode m);
    virtual double GetBeamLimit() const;
    virtual double GetBeamRange() const;
    virtual void IncreaseRange();
    virtual void DecreaseRange();
    virtual void AddRange(double r);

    Contact* FindContact(Ship* s);
    Contact* FindContact(Shot* s);

    // borrow this sensor for missile seeker
    SimObject* AcquirePassiveTargetForMissile();
    SimObject* AcquireActiveTargetForMissile();

    // SimObserver:
    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

  protected:
    void ProcessContact(Ship* contact, double az1, double az2);
    void ProcessContact(Shot* contact, double az1, double az2);

    Mode mode;
    int nsettings;
    int range_index;
    float range_settings[8];
    SimObject* target;

    List<Contact> contacts;
};

#endif Sensor_h
