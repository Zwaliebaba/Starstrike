#pragma once

#include "building.h"

class Refinery;
class MineCart;

// ****************************************************************************
// Class MineBuilding
// ****************************************************************************

class MineBuilding : public Building
{
  protected:
    int m_trackLink;
    ShapeMarkerData* m_trackMarker1;
    ShapeMarkerData* m_trackMarker2;

    Matrix34 m_trackMatrix1;
    Matrix34 m_trackMatrix2;

    LList<MineCart*> m_carts;

    float m_previousMineSpeed;
    float m_wheelRotate;

    static ShapeStatic* s_wheelShape;
    static ShapeStatic* s_cartShape;
    static ShapeMarkerData* s_cartMarker1;
    static ShapeMarkerData* s_cartMarker2;
    static ShapeMarkerData* s_cartContents[3];
    static ShapeStatic* s_polygon1;
    static ShapeStatic* s_primitive1;

    static float s_refineryPopulation;
    static float s_refineryRecalculateTimer;
    static float RefinerySpeed();

  public:
    MineBuilding();

    void Initialise(Building* _template) override;
    bool Advance() override;

    bool IsInView() override;

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;
    void RenderCart(MineCart* _cart, float _predictionTime);

    LegacyVector3 GetTrackMarker1();
    LegacyVector3 GetTrackMarker2();

    virtual void TriggerCart(MineCart* _cart, float _initValue);

    void ListSoundEvents(LList<const char*>* _list) override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;
};

class MineCart
{
  public:
    float m_progress; // Progress down current line, 0.0f - 1.0f

    bool m_polygons[3];
    bool m_primitives[3];

    MineCart();
};

// ****************************************************************************
// Class TrackLink
// ****************************************************************************

class TrackLink : public MineBuilding
{
  public:
    TrackLink();

    bool Advance() override;
};

// ****************************************************************************
// Class TrackJunction
// ****************************************************************************

class TrackJunction : public MineBuilding
{
  public:
    LList<int> m_trackLinks;

    TrackJunction();

    void Initialise(Building* _template) override;

    void Render(float _predictionTime) override;
    void TriggerCart(MineCart* _cart, float _initValue) override;

    void SetBuildingLink(int _buildingId) override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;
};

// ****************************************************************************
// Class TrackStart
// ****************************************************************************

class TrackStart : public MineBuilding
{
  public:
    int m_reqBuildingId; // This building must be online

    TrackStart();

    void Initialise(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;
};

// ****************************************************************************
// Class TrackEnd
// ****************************************************************************

class TrackEnd : public MineBuilding
{
  public:
    int m_reqBuildingId; // This building must be online

    TrackEnd();

    void Initialise(Building* _template) override;
    bool Advance() override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;
};

// ****************************************************************************
// Class Refinery
// ****************************************************************************

class Refinery : public MineBuilding
{
  protected:
    ShapeMarkerData* m_wheel1;
    ShapeMarkerData* m_wheel2;
    ShapeMarkerData* m_wheel3;
    ShapeMarkerData* m_counter1;

  public:
    Refinery();

    bool Advance() override;
    void Render(float _predictionTime) override;

    const char* GetObjectiveCounter() override;

    void TriggerCart(MineCart* _cart, float _initValue) override;
};

// ****************************************************************************
// Class Mine
// ****************************************************************************

class Mine : public MineBuilding
{
  protected:
    ShapeMarkerData* m_wheel1;
    ShapeMarkerData* m_wheel2;

  public:
    Mine();

    void Render(float _predictionTime) override;

    void TriggerCart(MineCart* _cart, float _initValue) override;
};

