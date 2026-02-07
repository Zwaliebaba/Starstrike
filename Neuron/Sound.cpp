#include "pch.h"
#include "Sound.h"
#include "SoundCard.h"
#include "Wave.h"

SoundCard* Sound::creator = nullptr;

Sound* Sound::CreateStream(const char* filename)
{
  Sound* sound = nullptr;

  if (!filename || !filename[0] || !creator)
    return sound;

  int namelen = strlen(filename);

  if (namelen < 5)
    return sound;

  WAVE_HEADER head;
  WAVE_FMT fmt;
  WAVE_FACT fact;
  WAVE_DATA data;
  WAVEFORMATEX wfex;

  ZeroMemory(&head, sizeof(head));
  ZeroMemory(&fmt, sizeof(fmt));
  ZeroMemory(&fact, sizeof(fact));
  ZeroMemory(&data, sizeof(data));

  LPBYTE buf = nullptr;
  LPBYTE p = nullptr;
  int len = 0;

  FILE* f;
  fopen_s(&f, filename, "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    len = std::min(len, 4096);

    buf = NEW BYTE[len];

    if (buf && len)
      fread(buf, len, 1, f);

    fclose(f);
  }

  if (len > sizeof(head))
  {
    CopyMemory(&head, buf, sizeof(head));

    if (head.RIFF == MAKEFOURCC('R', 'I', 'F', 'F') && head.WAVE == MAKEFOURCC('W', 'A', 'V', 'E'))
    {
      p = buf + sizeof(WAVE_HEADER);

      do
      {
        DWORD chunk_id = *((LPDWORD)p);

        switch (chunk_id)
        {
          case MAKEFOURCC('f', 'm', 't', ' '): CopyMemory(&fmt, p, sizeof(fmt));
            p += fmt.chunk_size + 8;
            break;

          case MAKEFOURCC('f', 'a', 'c', 't'): CopyMemory(&fact, p, sizeof(fact));
            p += fact.chunk_size + 8;
            break;

          case MAKEFOURCC('s', 'm', 'p', 'l'): CopyMemory(&fact, p, sizeof(fact));
            p += fact.chunk_size + 8;
            break;

          case MAKEFOURCC('d', 'a', 't', 'a'): CopyMemory(&data, p, sizeof(data));
            p += 8;
            break;

          default:
            delete[] buf;
            return sound;
        }
      }
      while (data.chunk_size == 0);

      wfex.wFormatTag = fmt.wFormatTag;
      wfex.nChannels = fmt.nChannels;
      wfex.nSamplesPerSec = fmt.nSamplesPerSec;
      wfex.nAvgBytesPerSec = fmt.nAvgBytesPerSec;
      wfex.nBlockAlign = fmt.nBlockAlign;
      wfex.wBitsPerSample = fmt.wBitsPerSample;
      wfex.cbSize = 0;

      sound = Create(STREAMED, &wfex);

      if (sound)
      {
        sound->SetFilename(filename);
        sound->StreamFile(filename, p - buf);
      }
    }
  }

  delete[] buf;
  return sound;
}

Sound* Sound::Create(DWORD flags, LPWAVEFORMATEX format)
{
  if (creator)
    return creator->CreateSound(flags, format);
  return nullptr;
}

Sound* Sound::Create(DWORD flags, LPWAVEFORMATEX format, DWORD len, LPBYTE data)
{
  if (creator)
    return creator->CreateSound(flags, format, len, data);
  return nullptr;
}

void Sound::SetListener(const Camera& cam, const Vec3& vel)
{
  if (creator)
    creator->SetListener(cam, vel);
}

Sound::Sound()
  : flags(0),
    status(UNINITIALIZED),
    volume(0),
    looped(0),
    location(0, 0, 0),
    velocity(0, 0, 0),
  filename("Sound()"),
    sound_check(nullptr) {  }

Sound::~Sound() {}

void Sound::Release() { flags &= ~LOCKED; }

void Sound::AddToSoundCard()
{
  if (creator)
    creator->AddSound(this);
}

void Sound::SetFilename(std::string_view s)
{
  if (!s.empty())
  {
    int n = s.length();

    if (n >= 60)
    {
      filename = std::string("...") + std::string(s.substr(n - 59));
    }

    else
      filename = s;
  }
}
