#ifndef Computer_h
#define Computer_h

#include "System.h"

class Computer : public System
{
  public:
    enum CompType
    {
      AVIONICS = 1,
      FLIGHT,
      TACTICAL
    };

    Computer(int comp_type, std::string_view comp_name);
    Computer(const Computer& rhs);
    ~Computer() override;

    void ApplyDamage(double damage) override;
    void ExecFrame(double seconds) override;
    void Distribute(double delivered_energy, double seconds) override;
};

#endif Computer_h
