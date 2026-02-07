#ifndef Starshatter_h
#define Starshatter_h

#include "Bitmap.h"
#include "Game.h"
#include "KeyMap.h"


class Campaign;
class MenuScreen;
class CmpnScreen;
class PlanScreen;
class LoadScreen;
class GameScreen;
class Ship;
class Sim;
class FadeView;
class CameraDirector;
class MultiController;
class MouseController;
class MusicDirector;
class DataLoader;
class Font;
class Mission;
class NetServer;
class NetLobby;

class Starshatter : public Game
{
  public:
    Starshatter();
    ~Starshatter() override;

    bool Init(HINSTANCE hi, HINSTANCE hpi, LPSTR cmdline, int nCmdShow) override;
    bool InitGame() override;
    virtual bool ChangeVideo();
    void GameState() override;
    void Exit() override;
    bool OnHelp() override;

    enum MODE
    {
      MENU_MODE,  // main menu
      CLOD_MODE,  // loading campaign
      CMPN_MODE,  // operational command for dynamic campaign
      PREP_MODE,  // loading mission info for planning
      PLAN_MODE,  // mission briefing
      LOAD_MODE,  // loading mission into simulator
      PLAY_MODE,  // active simulation
      EXIT_MODE   // shutting down
    };

    enum LOBBY
    {
      NET_LOBBY_CLIENT,
      NET_LOBBY_SERVER
    };

    int GetGameMode() { return m_gameMode; }
    void SetGameMode(int mode);
    void RequestChangeVideo();
    void LoadVideoConfig(const char* filename);
    void SaveVideoConfig(const char* filename);
    void SetupSplash();
    void SetupMenuScreen();
    void SetupCmpnScreen();
    void SetupPlanScreen();
    void SetupLoadScreen();
    void SetupGameScreen();
    void OpenTacticalReference();
    void CreateWorld();
    int KeyDown(int action) const;

    void PlayerCam(int mode);
    void ViewSelection();

    void MapKeys();
    static void MapKeys(KeyMap* mapping, int nkeys);
    static void MapKey(int act, int key, int alt = 0);

    void SetTestMode(int t) { test_mode = t; }

    static Starshatter* GetInstance() { return instance; }

    int GetScreenWidth();
    int GetScreenHeight();

    // graphic options:
    int LensFlare() { return lens_flare; }
    int Corona() { return corona; }
    int Nebula() { return nebula; }
    int Dust() { return dust; }

    KeyMap& GetKeyMap() { return keycfg; }

    int GetLoadProgress() { return load_progress; }
    std::string GetLoadActivity() { return load_activity; }

    void InvalidateTextureCache();

    int GetChatMode() const { return chat_mode; }
    void SetChatMode(int c);
    std::string GetChatText() const { return chat_text; }

    void StopNetGame();

    int GetLobbyMode();
    void SetLobbyMode(int mode = NET_LOBBY_CLIENT);
    void StartLobby();
    void StopLobby();

    void ExecCutscene(std::string_view msn_file, std::string_view path);
    void BeginCutscene();
    void EndCutscene();
    bool InCutscene() const { return cutscene > 0; }
    Mission* GetCutsceneMission() const;
    std::string GetSubtitles() const;
    void EndMission();

    void StartOrResumeGame();

  protected:
    virtual void DoMenuScreenFrame();
    virtual void DoCmpnScreenFrame();
    virtual void DoPlanScreenFrame();
    virtual void DoLoadScreenFrame();
    virtual void DoGameScreenFrame();
    virtual void DoMouseFrame();

    virtual void DoChatMode();
    virtual void DoGameKeys();

    bool GameLoop() override;
    void UpdateWorld() override;
    virtual void InstantiateMission();
    bool ResizeVideo() override;
    virtual void InitMouse();

    inline static Starshatter* instance = {};
    Window* gamewin;
    MenuScreen* menuscreen;
    LoadScreen* loadscreen;
    PlanScreen* planscreen;
    GameScreen* gamescreen;
    CmpnScreen* cmpnscreen;

    FadeView* splash;
    int splash_index;
    Bitmap splash_image;
    MultiController* input;
    MouseController* mouse_input;
    DataLoader* loader;

    Ship* player_ship;
    CameraDirector* cam_dir;
    MusicDirector* music_dir;

    Font* HUDfont;
    Font* GUIfont;
    Font* GUI_small_font;
    Font* terminal;
    Font* verdana;
    Font* title_font;
    Font* limerick18;
    Font* limerick12;
    Font* ocrb;

    DWORD time_mark;
    DWORD minutes;

    double field_of_view;
    double orig_fov;

    inline static int keymap[256];
    inline static int keyalt[256];
    KeyMap keycfg;

    bool tactical;
    bool spinning;
    int mouse_x;
    int mouse_y;
    int mouse_dx;
    int mouse_dy;

    int m_gameMode;
    int test_mode;
    int req_change_video;
    int video_changed;

    int lens_flare;
    int corona;
    int nebula;
    int dust;

    double exit_time;

    int load_step;
    int load_progress;
    std::string load_activity;
    int catalog_index;

    int cutscene;
    int lobby_mode;
    NetLobby* net_lobby;
    int chat_mode;
    std::string chat_text;
};

#endif Starshatter_h
