#pragma once

#include "GuiWindow.h"

class CheatWindow : public GuiWindow
{
  public:
    CheatWindow(const char* _name);

    void Create() override;
};
