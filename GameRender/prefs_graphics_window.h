#pragma once

#include "darwinia_window.h"

class PrefsGraphicsWindow : public DarwiniaWindow
{
  public:
    int m_landscapeDetail;
    int m_cloudDetail;
    int m_buildingDetail;
    int m_entityDetail;

    PrefsGraphicsWindow();
    ~PrefsGraphicsWindow() override;

    void Create() override;
    void Render(bool _hasFocus) override;
};
