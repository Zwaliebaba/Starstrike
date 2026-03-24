#pragma once

#include "GameContext.h"

class GameApp : public GameMain, public GameContext
{
  public:
    GameApp();
    ~GameApp() override;

    // --- GameMain overrides ---
    void Startup() override {}
    void Shutdown() override {}
    void Update([[maybe_unused]] float _deltaT) override {}
    void RenderScene() override {}

    void OnActivated() override;
    void OnDeactivated() override;
    void OnSuspending() override;
    void OnResuming() override;

    // --- Lifecycle methods ---
    void SetProfileName(const char* _profileName);
    bool LoadProfile();
    void ResetLevel(bool _global);
    void SetLanguage(const char* _language, bool _test);
    void LoadPrologue();
    void LoadCampaign();
    void UpdateDifficultyFromPreferences();
};

extern GameApp* g_gameApp;

