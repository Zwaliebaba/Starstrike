#pragma once

#include "building.h"

#define TRUNKPORT_HEIGHTMAP_MAXSIZE 16

class TrunkPort : public Building
{
  public:
    int m_targetLocationId;

    ShapeMarkerData* m_destination1;
    ShapeMarkerData* m_destination2;

    int m_heightMapSize;
    LegacyVector3* m_heightMap;
    float m_openTimer;

    TrunkPort();

    void Initialise(Building* _template) override;
    void SetDetail(int _detail) override;
    bool Advance() override;
    void Render(float predictionTime) override;
    void RenderAlphas(float predictionTime) override;

    bool PerformDepthSort(LegacyVector3& _centerPos) override;

    void ReprogramComplete() override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* _out) override;
};
