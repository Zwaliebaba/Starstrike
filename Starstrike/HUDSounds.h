#ifndef HUDSounds_h
#define HUDSounds_h

#undef PlaySound

class HUDSounds
{
  public:
    enum SOUNDS
    {
      SND_MFD_MODE,
      SND_NAV_MODE,
      SND_WEP_MODE,
      SND_WEP_DISP,
      SND_HUD_MODE,
      SND_HUD_WIDGET,
      SND_SHIELD_LEVEL,
      SND_RED_ALERT,
      SND_TAC_ACCEPT,
      SND_TAC_REJECT
    };

    static void Initialize();
    static void Close();

    static void PlaySound(int n);
    static void StopSound(int n);
};

#endif HUDSounds_h
