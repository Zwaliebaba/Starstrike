#ifndef Skin_h
#define Skin_h

#include "List.h"
#include "Polygon.h"

class Solid;
class Model;
class Surface;
class Segment;

class Skin;
class SkinCell;

class Skin
{
  public:
    enum { NAMELEN = 64 };

    Skin(std::string_view name = {});
    virtual ~Skin();

    // operations
    void ApplyTo(Model* model) const;
    void Restore(Model* model) const;

    // accessors / mutators
    std::string Name() const { return name; }
    std::string Path() const { return path; }
    int NumCells() const { return cells.size(); }

    void SetName(std::string_view n);
    void SetPath(std::string_view n);
    void AddMaterial(const Material* mtl);

  protected:
    std::string name;
    std::string path;
    List<SkinCell> cells;
};

class SkinCell
{
  friend class Skin;

  public:
    SkinCell(const Material* mtl = nullptr);
    ~SkinCell();

    int operator ==(const SkinCell& other) const;

    std::string Name() const;
    const Material* Skin() const { return skin; }
    const Material* Orig() const { return orig; }

    void SetSkin(const Material* mtl);
    void SetOrig(const Material* mtl);

  private:
    const Material* skin;
    const Material* orig;
};

#endif Skin_h
