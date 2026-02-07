#pragma once

#include "Geometry.h"
#include "Shot.h"
#include "SimObject.h"
#include "System.h"

class Contact : public SimObserver
{
  friend class Sensor;

  public:
    
    Contact();
    Contact(Ship* s, float p, float a);
    Contact(Shot* s, float p, float a);
    ~Contact() override;

    int operator ==(const Contact& c) const;

    Ship* GetShip() const { return ship; }
    Shot* GetShot() const { return shot; }
    Point Location() const { return loc; }

    double PasReturn() const { return d_pas; }
    double ActReturn() const { return d_act; }
    bool PasLock() const;
    bool ActLock() const;
    double Age() const;
    bool IsProbed() const { return probe; }

    DWORD AcquisitionTime() const { return acquire_time; }

    int GetIFF(const Ship* observer) const;
    void GetBearing(const Ship* observer, double& az, double& el, double& r) const;
    double Range(const Ship* observer, double limit = 75e3) const;

    bool InFront(const Ship* observer) const;
    bool Threat(const Ship* observer) const;
    bool Visible(const Ship* observer) const;

    void Reset();
    void Merge(Contact* c);
    void ClearTrack();
    void UpdateTrack();
    int TrackLength() const { return ntrack; }
    Point TrackPoint(int i) const;

    bool Update(SimObject* obj) override;
    std::string GetObserverName() const override;

  private:
    Ship* ship;
    Shot* shot;
    Point loc;
    DWORD acquire_time;
    DWORD time;

    Point* track;
    int ntrack;
    DWORD track_time;

    float d_pas;   // power output
    float d_act;   // mass, size
    bool probe;   // scanned by probe
};
