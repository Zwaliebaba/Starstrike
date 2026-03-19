#pragma once

#include "fast_darray.h"
#include "text_stream_readers.h"
#include "building.h"
#include "spirit.h"

class ShapeMarkerData;

#define INCUBATOR_PROCESSTIME       5.0f

struct IncubatorIncoming
{
  LegacyVector3 m_pos;
  int m_entrance;
  float m_alpha;
};

class Incubator : public Building
{
  protected:
    FastDArray<Spirit> m_spirits;
    ShapeMarkerData* m_spiritCenter;
    ShapeMarkerData* m_exit;
    ShapeMarkerData* m_dock;
    ShapeMarkerData* m_spiritEntrance[3];

    int m_troopType;
    float m_timer;

    LList<IncubatorIncoming*> m_incoming;

  public:
    int m_numStartingSpirits;

    Incubator();
    ~Incubator() override;

    void Initialise(Building* _template) override;

    bool Advance() override;
    void SpawnEntity();
    void AddSpirit(Spirit* _spirit);

    void Render(float _predictionTime) override;
    void RenderAlphas(float _predictionTime) override;

    int NumSpiritsInside();

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;

    void GetDockPoint(LegacyVector3& _pos, LegacyVector3& _front);
};
