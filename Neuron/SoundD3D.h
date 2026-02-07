#ifndef SoundD3D_h
#define SoundD3D_h

//#define DIRECT_SOUND_3D
#include <dsound.h>
#include <stdio.h>
#include "Camera.h"
#include "Sound.h"
#include "SoundCard.h"


class SoundD3D;
class SoundCardD3D;

// Sound Implementation for DirectSound and DirectSound3D

class SoundD3D : public Sound
{
  public:
    
    SoundD3D(LPDIRECTSOUND card, DWORD flags, LPWAVEFORMATEX format);
    SoundD3D(LPDIRECTSOUND card, DWORD flags, LPWAVEFORMATEX format, DWORD len, LPBYTE data);
    ~SoundD3D() override;

    void Update() override;

    HRESULT StreamFile(const char* name, DWORD offset) override;
    HRESULT Load(DWORD bytes, BYTE* data) override;
    HRESULT Play() override;
    HRESULT Rewind() override;
    HRESULT Pause() override;
    HRESULT Stop() override;

    Sound* Duplicate() override;

    // (only for streamed sounds)
    double GetTotalTime() const override { return total_time; }
    double GetTimeRemaining() const override;
    double GetTimeElapsed() const override;

    // (only used for localized sounds)
    void SetVolume(long v) override;
    long GetPan() const override;
    void SetPan(long p) override;
    void SetLocation(const Vec3& l) override;
    void SetVelocity(const Vec3& v) override;

    float GetMinDistance() const override;
    void SetMinDistance(float f) override;
    float GetMaxDistance() const override;
    void SetMaxDistance(float f) override;

  protected:
    void Localize();
    HRESULT AllocateBuffer(DWORD bytes);

    void StreamBlock();
    void RewindStream();

    LPDIRECTSOUND soundcard;
    WAVEFORMATEX wfex;
    DSBUFFERDESC dsbd;
    LPDIRECTSOUNDBUFFER buffer;

    DWORD data_len;
    LPBYTE data;

#ifdef DIRECT_SOUND_3D
    LPDIRECTSOUND3DBUFFER      sound3d;
#endif

    float min_dist;
    float max_dist;

    // STREAMED SOUND SUPPORT:
    FILE* stream;
    DWORD stream_left;
    double total_time;
    DWORD min_safety;
    DWORD read_size;
    BYTE* transfer;
    DWORD w, r;
    DWORD stream_offset;
    bool eos_written;
    BYTE eos_latch;
    bool moved;

    std::mutex sync;
};

// Sound Card Implementation for DS and DS3D

class SoundCardD3D : public SoundCard
{
  friend class SoundD3D;

  public:
    
    SoundCardD3D(HWND hwnd);
    ~SoundCardD3D() override;

    // Format of the sound card's primary buffer:
    bool GetFormat(LPWAVEFORMATEX format) override;
    bool SetFormat(LPWAVEFORMATEX format) override;
    bool SetFormat(int bits, int channels, int hertz) override;

    virtual void ShowFormat();

    // Get a blank, writable sound buffer:
    Sound* CreateSound(DWORD flags, LPWAVEFORMATEX format) override;

    // Create a sound resource:
    Sound* CreateSound(DWORD flags, LPWAVEFORMATEX format, DWORD len, LPBYTE data) override;

    void SetListener(const Camera& cam, const Vec3& vel) override;
    bool Pause() override;
    bool Resume() override;
    bool StopSoundEffects() override;

  protected:
    LPDIRECTSOUND soundcard;
    LPDIRECTSOUNDBUFFER primary;

#ifdef DIRECT_SOUND_3D
    LPDIRECTSOUND3DLISTENER    listener;
#else
    Camera listener;
    Vec3 velocity;
#endif

    WAVEFORMATEX wfex;
    DSBUFFERDESC dsbd;
};

#endif SoundD3D_h
