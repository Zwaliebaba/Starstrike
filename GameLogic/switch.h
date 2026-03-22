#pragma once

#include "building.h"

class ShapeStatic;
class ShapeFragmentData;
class ShapeMarkerData;

class FenceSwitch : public Building
{
    friend class FenceSwitchBuildingRenderer;
  protected:
    int m_linkedBuildingId;
    int m_linkedBuildingId2; // optional second link for fence toggling

    bool m_switchable;

    float m_timer; // no fences changes will be made until this timer counts to 0 for the first time

    ShapeMarkerData* m_connectionLocation;

  public:
    char m_script[256];
    bool m_locked;
    int m_lockable;
    int m_switchValue;

    FenceSwitch();

    void Initialise(Building* _template) override;
    void SetDetail(int _detail) override;

    bool Advance() override;

    void Switch();

    int GetBuildingLink() override;
    void SetBuildingLink(int _buildingId) override;

    void Read(TextReader* _in, bool _dynamic) override;
    void Write(FileWriter* out) override;

    LegacyVector3 GetConnectionLocation();

    bool IsInView() override;
};

