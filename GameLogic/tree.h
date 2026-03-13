#pragma once

#include "building.h"

class Tree : public Building
{
  protected:
    int m_branchDisplayListId;
    int m_leafDisplayListId;
    void RenderBranch(LegacyVector3 _from, LegacyVector3 _to, int _iterations, bool _calcRadius, bool _renderBranch, bool _renderLeaf);

    LegacyVector3 m_hitcheckCentre;
    float m_hitcheckRadius;
    int m_numLeafs;

    float m_fireDamage;
    float m_onFire;
    bool m_burnSoundPlaying;

    float GetActualHeight(float _predictionTime);

    unsigned char m_leafColourArray[4];
    unsigned char m_branchColourArray[4];

  public:
    float m_height;
    float m_budsize;
    float m_pushUp;
    float m_pushOut;
    int m_iterations;
    int m_seed;
    int m_leafColour;
    int m_branchColour;
    int m_leafDropRate;

    Tree();
    ~Tree() override;

    void Initialise(Building* _template) override;
    void SetDetail(int _detail) override;

    bool Advance() override;

    void DeleteDisplayLists();
    void Generate();
    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    bool PerformDepthSort(LegacyVector3& _centrePos) override;

    void Damage(float _damage) override;

    bool DoesSphereHit(const LegacyVector3& _pos, float _radius) override;
    bool DoesShapeHit(Shape* _shape, Matrix34 _transform) override;
    bool DoesRayHit(const LegacyVector3& _rayStart, const LegacyVector3& _rayDir, float _rayLen = 1e10, LegacyVector3* _pos = nullptr,
                    LegacyVector3* _norm = nullptr) override; // pos/norm will not always be available

    void ListSoundEvents(LList<char*>* _list) override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;
};

