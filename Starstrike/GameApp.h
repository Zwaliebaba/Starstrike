#pragma once

#include "GameAppSim.h"

class GameApp : public GameAppSim, public GameMain
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

    // --- GameAppSim overrides ---
    void SetProfileName(const char* _profileName) override;
    bool LoadProfile() override;
    void ResetLevel(bool _global) override;
    void SetLanguage(const char* _language, bool _test) override;
    void LoadPrologue() override;
    void LoadCampaign() override;
    void UpdateDifficultyFromPreferences() override;
};

