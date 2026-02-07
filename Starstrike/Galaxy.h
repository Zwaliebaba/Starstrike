#ifndef Galaxy_h
#define Galaxy_h

#include "List.h"


class Star;
class StarSystem;
class Graphic;
class Light;
class Scene;

class Galaxy
{
  public:
    Galaxy(std::string_view name);
    virtual ~Galaxy();

    int operator ==(const Galaxy& s) const { return name == s.name; }

    // operations:
    virtual void Load();
    virtual void Load(std::string_view filename);
    virtual void ExecFrame();

    // accessors:
    std::string Name() const { return name; }
    std::string Description() const { return description; }
    List<StarSystem>& GetSystemList() { return systems; }
    List<Star>& Stars() { return stars; }
    double Radius() const { return radius; }

    StarSystem* GetSystem(std::string_view _name);
    StarSystem* FindSystemByRegion(std::string_view _rgnName);

    static void Initialize();
    static void Close();
    static Galaxy* GetInstance();

  protected:
    std::string m_filename;
    std::string name;
    std::string description;
    double radius;           // radius in parsecs

    List<StarSystem> systems;
    List<Star> stars;
};

#endif Galaxy_h
