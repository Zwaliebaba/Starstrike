#ifndef MachineInfo_h
#define MachineInfo_h

class MachineInfo
{
  public:
    enum
    {
      CPU_INVALID,
      CPU_P5 = 5,
      CPU_P6 = 6,
      CPU_P7 = 7,
      CPU_PLUS
    };

    enum
    {
      OS_INVALID,
      OS_WIN95,
      OS_WIN98,
      OS_WINNT,
      OS_WIN2K,
      OS_WINXP,
      OS_WINXP64,
      OS_WINVISTA,
      OS_WINSEVEN,
      OS_WINFUTURE
    };

    enum
    {
      DX_NONE,
      DX_3 = 3,
      DX_5 = 5,
      DX_6 = 6,
      DX_7 = 7,
      DX_8 = 8,
      DX_9 = 9,
      DX_PLUS
    };

    static int GetCpuClass();
    static int GetTotalRam();
    static int GetPlatform();
    static int GetDirectXVersion();

    static const wchar_t* GetShortDescription();
};

#endif MachineInfo_h
