#ifndef Scene_h
#define Scene_h

#include "Color.h"
#include "Geometry.h"
#include "List.h"

class Graphic;
class Light;

class Scene
{
  public:
    Scene() = default;
    virtual ~Scene();

    void AddBackground(Graphic* g);
    void DelBackground(Graphic* g);
    void AddForeground(Graphic* g);
    void DelForeground(Graphic* g);
    void AddGraphic(Graphic* g);
    void DelGraphic(Graphic* g);
    void AddSprite(Graphic* g);
    void DelSprite(Graphic* g);

    void AddLight(Light* l);
    void DelLight(Light* l);

    List<Graphic>& Background() { return background; }
    List<Graphic>& Foreground() { return foreground; }
    List<Graphic>& Graphics() { return graphics; }
    List<Graphic>& Sprites() { return sprites; }
    List<Light>& Lights() { return lights; }
    Color Ambient() { return ambient; }
    void SetAmbient(Color a) { ambient = a; }

    virtual void Collect();

    virtual bool IsLightObscured(const Point& obj_pos, const Point& light_pos, double obj_radius,
                                 Point* imp_point = nullptr) const;

  protected:
    List<Graphic> background;
    List<Graphic> foreground;
    List<Graphic> graphics;
    List<Graphic> sprites;
    List<Light> lights;
    Color ambient;
};

#endif Scene_h
