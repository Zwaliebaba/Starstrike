#ifndef HardPoint_h
#define HardPoint_h

#include "Geometry.h"

class Weapon;
class WeaponDesign;

class HardPoint
{
  public:
    enum CONSTANTS { MAX_DESIGNS = 8 };

    HardPoint(Vec3 muzzle, double az = 0, double el = 0);
    HardPoint(const HardPoint& rhs);
    virtual ~HardPoint();

    int operator==(const HardPoint& w) const { return this == &w; }

    virtual void AddDesign(WeaponDesign* dsn);
    virtual Weapon* CreateWeapon(int type_index = 0);
    virtual double GetCarryMass(int type_index = 0);
    WeaponDesign* GetWeaponDesign(int n) { return designs[n]; }

    virtual void Mount(Point loc, float rad, float hull = 0.5f);
    Point MountLocation() const { return mount_rel; }
    double Radius() const { return radius; }
    double HullProtection() const { return hull_factor; }

    [[nodiscard]] virtual std::string GetName() const { return name; }
    virtual void SetName(std::string_view s) { name = s; }
    [[nodiscard]] virtual std::string GetAbbreviation() const { return abrv; }
    virtual void SetAbbreviation(std::string_view s) { abrv = s; }
    [[nodiscard]] virtual std::string GetDesign() const { return sys_dsn; }
    virtual void SetDesign(std::string_view s) { sys_dsn = s; }

    [[nodiscard]] virtual double GetAzimuth() const { return aim_azimuth; }
    virtual void SetAzimuth(double a) { aim_azimuth = static_cast<float>(a); }
    [[nodiscard]] virtual double GetElevation() const { return aim_elevation; }
    virtual void SetElevation(double e) { aim_elevation = static_cast<float>(e); }

  protected:
    // Displayable name:
    std::string name;
    std::string abrv;
    std::string sys_dsn;

    WeaponDesign* designs[MAX_DESIGNS];
    Vec3 muzzle;
    float aim_azimuth;
    float aim_elevation;

    // Mounting:
    Point mount_rel;  // object space
    float radius;
    float hull_factor;
};

#endif HardPoint_h
