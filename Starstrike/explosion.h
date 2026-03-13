#ifndef INCLUDED_EXPLOSION
#define INCLUDED_EXPLOSION

#include "matrix33.h"
#include "rgb_colour.h"
#include "shape.h"

//*****************************************************************************
// Class Tumbler
//*****************************************************************************

// This class maintains a rotation matrix that is used to make an ExplodingTri
// tumble as it flys through space. The m_rotMat matrix has a little bit of
// rotation multiplied on to it every frame.
class Tumbler
{
  public:
    Matrix33 m_rotMat;
    LegacyVector3 m_angVel;

    Tumbler();
    void Advance();
};

//*****************************************************************************
// Class ExplodingTri
//*****************************************************************************

class ExplodingTri
{
  public:
    LegacyVector3 m_vel;
    LegacyVector3 m_pos;
    LegacyVector3 m_v1, m_v2, m_v3;
    LegacyVector3 m_norm;
    Tumbler* m_tumbler;          // Non-owning; points into parent Explosion::m_tumblers
    float m_timeToDie;
    RGBAColour m_colour;
};

//*****************************************************************************
// Class Explosion
//*****************************************************************************

class Explosion
{
  protected:
    std::vector<Tumbler> m_tumblers;
    std::vector<ExplodingTri> m_tris;
    float m_timeToDie;

  public:
    Explosion(ShapeFragment* _frag, const Matrix34& _transform, float _fraction);
    ~Explosion() = default;
    Explosion(const Explosion&) = delete;
    Explosion& operator=(const Explosion&) = delete;

    bool Advance(); // Returns true if the explosion has finished
    void Render() const;

    LegacyVector3 GetCenter() const;
};

//*****************************************************************************
// Class ExplosionManager
//*****************************************************************************

class ExplosionManager
{
  protected:
    std::vector<Explosion*> m_explosions;

  public:
    ExplosionManager();
    ~ExplosionManager();

    void AddExplosion(ShapeFragment* _frag, const Matrix34& _transform, bool _recurse, float _fraction);
    void AddExplosion(const Shape* _shape, const Matrix34& _transform, float _fraction = 1.0f);
    void Reset();

    void Advance();
    void Render();

    const std::vector<Explosion*>& GetExplosionList() { return m_explosions; } // read access for DeformEffect
};

inline ExplosionManager g_explosionManager;

#endif
