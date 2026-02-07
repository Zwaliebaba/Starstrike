#ifndef SoundCard_h
#define SoundCard_h

#include "List.h"
#include "Sound.h"


class SoundCard
{
  public:
    
    SoundCard();
    virtual ~SoundCard();

    enum SoundStatus
    {
      SC_UNINITIALIZED,
      SC_OK,
      SC_ERROR,
      SC_BAD_PARAM
    };

    SoundStatus Status() const { return status; }

    // Format of the sound card's primary buffer:
    virtual bool GetFormat([[maybe_unused]] LPWAVEFORMATEX format) { return false; }
    virtual bool SetFormat([[maybe_unused]] LPWAVEFORMATEX format) { return false; }
    virtual bool SetFormat(int bits, int channels, int hertz) { return false; }
    virtual bool Pause() { return false; }
    virtual bool Resume() { return false; }
    virtual bool StopSoundEffects() { return false; }

    // Get a blank, writable sound buffer:
    virtual Sound* CreateSound(DWORD flags, LPWAVEFORMATEX format) { return nullptr; }

    // Create a sound resource:
    virtual Sound* CreateSound(DWORD flags, LPWAVEFORMATEX format, DWORD len, LPBYTE data) { return nullptr; }

    // once per frame:
    virtual void Update();

    virtual void SetListener([[maybe_unused]] const Camera& cam, [[maybe_unused]] const Vec3& vel) {}
    virtual DWORD UpdateThread();
    virtual void AddSound(Sound* s);

  protected:
    bool shutdown;
    HANDLE hthread;
    SoundStatus status;
    List<Sound> sounds;
    std::mutex sync;
};

#endif SoundCard_h
