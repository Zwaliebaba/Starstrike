#ifndef Intel_h
#define Intel_h

class Intel
{
  public:
    enum INTEL_TYPE
    {
      RESERVE = 1,   // out-system reserve: this group is not even here
      SECRET,        // enemy is completely unaware of this group
      KNOWN,         // enemy knows this group is in the system
      LOCATED,       // enemy has located at least the lead ship
      TRACKED        // enemy is tracking all elements
    };

    static int IntelFromName(std::string_view name);
    static std::string NameFromIntel(int intel);
};

#endif Intel_h
