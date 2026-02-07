#ifndef MusicDirector_h
#define MusicDirector_h

class MusicTrack;

class MusicDirector
{
  public:
    enum MODES
    {
      NONE,

      // menu modes:

      MENU,
      INTRO,
      BRIEFING,
      DEBRIEFING,
      PROMOTION,
      VICTORY,
      DEFEAT,
      CREDITS,

      // in game modes:

      FLIGHT,
      COMBAT,
      LAUNCH,
      RECOVERY,

      // special modes:
      SHUTDOWN
    };

    enum TRANSITIONS
    {
      CUT,
      FADE_OUT,
      FADE_IN,
      FADE_BOTH,
      CROSS_FADE
    };

    MusicDirector();
    ~MusicDirector();

    // Operations:
    void ExecFrame();
    void ScanTracks();

    int CheckMode(int mode);
    int GetMode() const { return mode; }

    static void Initialize();
    static void Close();
    static MusicDirector* GetInstance();
    static void SetMode(int mode);
    static const char* GetModeName(int mode);
    static bool IsNoMusic();

  protected:
    void StartThread();
    void StopThread();
    void GetNextTrack(int index);
    void ShuffleTracks();

    int mode;
    int transition;

    MusicTrack* track;
    MusicTrack* next_track;

    std::vector<std::string> menu_tracks;
    std::vector<std::string> intro_tracks;
    std::vector<std::string> brief_tracks;
    std::vector<std::string> debrief_tracks;
    std::vector<std::string> promote_tracks;
    std::vector<std::string> flight_tracks;
    std::vector<std::string> combat_tracks;
    std::vector<std::string> launch_tracks;
    std::vector<std::string> recovery_tracks;
    std::vector<std::string> victory_tracks;
    std::vector<std::string> defeat_tracks;
    std::vector<std::string> credit_tracks;

    bool no_music;

    HANDLE hproc;
    std::mutex m_mutex;
};

#endif MusicDirector_h
