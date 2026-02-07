#include "pch.h"
#include "RadioVox.h"
#include "AudioConfig.h"
#include "DataLoader.h"
#include "RadioView.h"
#include "Sound.h"

//
// RADIO VOX CONTROLLER:
//

DWORD WINAPI VoxUpdateProc(LPVOID link);

class RadioVoxController
{
  public:
    enum { MAX_QUEUE = 5 };

    RadioVoxController();
    ~RadioVoxController();

    bool Add(RadioVox* vox);
    void Update();
    DWORD UpdateThread();

    bool shutdown;
    HANDLE hthread;
    List<RadioVox> queue;
    std::mutex sync;
};

static RadioVoxController* g_controller = nullptr;

RadioVoxController::RadioVoxController()
  : shutdown(false),
    hthread(nullptr)
{
  DWORD thread_id = 0;
  hthread = CreateThread(nullptr, 4096, VoxUpdateProc, this, 0, &thread_id);
}

RadioVoxController::~RadioVoxController()
{
  shutdown = true;

  WaitForSingleObject(hthread, 500);
  CloseHandle(hthread);
  hthread = nullptr;

  queue.destroy();
}

DWORD WINAPI VoxUpdateProc(LPVOID link)
{
  auto controller = static_cast<RadioVoxController*>(link);

  if (controller)
    return controller->UpdateThread();

  return static_cast<DWORD>(E_POINTER);
}

DWORD RadioVoxController::UpdateThread()
{
  while (!shutdown)
  {
    Update();
    Sleep(50);
  }

  return 0;
}

void RadioVoxController::Update()
{
  std::unique_lock a(sync);

  if (queue.size())
  {
    RadioVox* vox = queue.first();

    if (!vox->Update())
      delete queue.removeIndex(0);
  }
}

bool RadioVoxController::Add(RadioVox* vox)
{
  if (!vox || vox->sounds.isEmpty())
    return false;

  std::unique_lock a(sync);

  if (queue.size() < MAX_QUEUE)
  {
    queue.append(vox);
    return true;
  }

  return false;
}

//
// RADIO VOX MESSAGE:
//

void RadioVox::Initialize()
{
  if (!g_controller)
    g_controller = NEW RadioVoxController;
}

void RadioVox::Close()
{
  delete g_controller;
  g_controller = nullptr;
}

RadioVox::RadioVox(int n, const char* p, const char* m)
  : path(p),
    message(m),
    index(0),
    channel(n) {}

RadioVox::~RadioVox() { sounds.destroy(); }

bool RadioVox::AddPhrase(const char* key)
{
  if (AudioConfig::VoxVolume() <= AudioConfig::Silence())
    return false;

  DataLoader* loader = DataLoader::GetLoader();
  if (!loader)
    return false;

  if (key && *key)
  {
    char datapath[256];
    char filename[256];

    sprintf_s(datapath, "Vox\\%s\\", path.data());
    sprintf_s(filename, "%s.wav", key);

    Sound* sound = nullptr;

    loader->SetDataPath(datapath);
    loader->LoadSound(filename, sound, Sound::LOCALIZED, true); // optional sound
    loader->SetDataPath();

    if (sound)
    {
      sound->SetVolume(AudioConfig::VoxVolume());
      sound->SetFlags(Sound::LOCALIZED | Sound::LOCKED);
      sound->SetFilename(filename);
      sounds.append(sound);

      return true;
    }
  }

  return false;
}

bool RadioVox::Start()
{
  if (g_controller)
    return g_controller->Add(this);

  return false;
}

bool RadioVox::Update()
{
  if (message.length())
  {
    RadioView::Message(message);
    message = "";
  }

  bool active = false;

  while (!active && index < sounds.size())
  {
    Sound* s = sounds[index];

    if (s->IsReady())
    {
      if (channel & 1)
        s->SetPan(channel * -3000);
      else
        s->SetPan(channel * 3000);

      s->Play();
      active = true;
    }

    else if (s->IsPlaying())
    {
      s->Update();
      active = true;
    }

    else
      index++;
  }

  return active;
}
