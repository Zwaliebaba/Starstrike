#include "pch.h"
#include "MusicDirector.h"
#include <random>
#include "DataLoader.h"
#include "MusicTrack.h"

static MusicDirector* music_director = nullptr;

MusicDirector::MusicDirector()
  : mode(0),
  transition(0),
  track(nullptr),
  next_track(nullptr),
  no_music(true),
  hproc(nullptr)
{
  music_director = this;

  ScanTracks();

  if (!no_music)
    StartThread();
}

MusicDirector::~MusicDirector()
{
  StopThread();

  delete track;
  delete next_track;

  if (this == music_director)
    music_director = nullptr;
}

void MusicDirector::Initialize()
{
  delete music_director;
  music_director = NEW MusicDirector();
}

void MusicDirector::Close()
{
  delete music_director;
  music_director = nullptr;
}

MusicDirector* MusicDirector::GetInstance() { return music_director; }

void MusicDirector::ExecFrame()
{
  if (no_music)
    return;

  std::unique_lock a(m_mutex);

  if (next_track && !track)
  {
    track = next_track;
    next_track = nullptr;
  }

  if (track)
  {
    if (track->IsDone())
    {
      if (mode != NONE && mode != SHUTDOWN && next_track == nullptr)
        GetNextTrack(track->GetIndex() + 1);

      delete track;
      track = next_track;
      next_track = nullptr;
    }

    else if (track->IsLooped())
    {
      if (mode != NONE && mode != SHUTDOWN && next_track == nullptr)
        GetNextTrack(track->GetIndex() + 1);

      track->FadeOut();
      track->ExecFrame();
    }

    else
      track->ExecFrame();
  }

  if (next_track)
  {
    if (next_track->IsDone())
    {
      delete next_track;
      next_track = nullptr;
    }

    else if (next_track->IsLooped())
    {
      next_track->FadeOut();
      next_track->ExecFrame();
    }

    else
      next_track->ExecFrame();
  }
}

void MusicDirector::ScanTracks()
{
  DataLoader* loader = DataLoader::GetLoader();

  loader->SetDataPath("Music\\");

  std::vector<std::string> files;
  loader->ListFiles("*.wav", files, true);

  if (files.size() == 0)
  {
    no_music = true;
    return;
  }

  no_music = false;

  for (auto& name : files)
  {
    std::string file = "Music\\" + name;

    if (name.starts_with("Menu"))
      menu_tracks.emplace_back(file);

    else if (name.starts_with("Intro"))
      intro_tracks.emplace_back(file);

    else if (name.starts_with("Brief"))
      brief_tracks.emplace_back(file);

    else if (name.starts_with("Debrief"))
      debrief_tracks.emplace_back(file);

    else if (name.starts_with("Promot"))
      promote_tracks.emplace_back(file);

    else if (name.starts_with("Flight"))
      flight_tracks.emplace_back(file);

    else if (name.starts_with("Combat"))
      combat_tracks.emplace_back(file);

    else if (name.starts_with("Launch"))
      launch_tracks.emplace_back(file);

    else if (name.starts_with("Defeat"))
      defeat_tracks.emplace_back(file);

    else if (name.starts_with("Victory"))
      victory_tracks.emplace_back(file);

    else if (name.starts_with("Credit"))
      credit_tracks.emplace_back(file);

    else
      menu_tracks.emplace_back(file);
  }

  std::ranges::sort(menu_tracks);
  std::ranges::sort(intro_tracks);
  std::ranges::sort(brief_tracks);
  std::ranges::sort(debrief_tracks);
  std::ranges::sort(promote_tracks);
  std::ranges::sort(flight_tracks);
  std::ranges::sort(combat_tracks);
  std::ranges::sort(launch_tracks);
  std::ranges::sort(recovery_tracks);
  std::ranges::sort(victory_tracks);
  std::ranges::sort(defeat_tracks);
  std::ranges::sort(credit_tracks);
}

const char* MusicDirector::GetModeName(int mode)
{
  switch (mode)
  {
  case NONE:
    return "NONE";
  case MENU:
    return "MENU";
  case INTRO:
    return "INTRO";
  case BRIEFING:
    return "BRIEFING";
  case DEBRIEFING:
    return "DEBRIEFING";
  case PROMOTION:
    return "PROMOTION";
  case FLIGHT:
    return "FLIGHT";
  case COMBAT:
    return "COMBAT";
  case LAUNCH:
    return "LAUNCH";
  case RECOVERY:
    return "RECOVERY";
  case VICTORY:
    return "VICTORY";
  case DEFEAT:
    return "DEFEAT";
  case CREDITS:
    return "CREDITS";
  case SHUTDOWN:
    return "SHUTDOWN";
  }

  return "UNKNOWN?";
}

void MusicDirector::SetMode(int mode)
{
  if (!music_director || music_director->no_music)
    return;

  std::unique_lock a(music_director->m_mutex);

  // stay in intro mode until it is complete:
  if (mode == MENU && (music_director->GetMode() == NONE || music_director->GetMode() == INTRO))
    mode = INTRO;

  mode = music_director->CheckMode(mode);

  if (mode != music_director->mode)
  {
    DebugTrace("MusicDirector::SetMode() old: %s  new: %s\n", GetModeName(music_director->mode), GetModeName(mode));

    music_director->mode = mode;

    MusicTrack* t = music_director->track;
    if (t && t->GetState() && !t->IsDone())
    {
      if (mode == NONE || mode == SHUTDOWN)
        t->SetFadeTime(0.5);

      t->FadeOut();
    }

    t = music_director->next_track;
    if (t && t->GetState() && !t->IsDone())
    {
      if (mode == NONE || mode == SHUTDOWN)
        t->SetFadeTime(0.5);
      t->FadeOut();

      delete music_director->track;
      music_director->track = t;
      music_director->next_track = nullptr;
    }

    music_director->ShuffleTracks();
    music_director->GetNextTrack(0);

    if (music_director->next_track)
      music_director->next_track->FadeIn();
  }
}

int MusicDirector::CheckMode(int req_mode)
{
  if (req_mode == RECOVERY && recovery_tracks.size() == 0)
    req_mode = LAUNCH;

  if (req_mode == LAUNCH && launch_tracks.size() == 0)
    req_mode = FLIGHT;

  if (req_mode == COMBAT && combat_tracks.size() == 0)
    req_mode = FLIGHT;

  if (req_mode == FLIGHT && flight_tracks.size() == 0)
    req_mode = NONE;

  if (req_mode == PROMOTION && promote_tracks.size() == 0)
    req_mode = VICTORY;

  if (req_mode == DEBRIEFING && debrief_tracks.size() == 0)
    req_mode = BRIEFING;

  if (req_mode == BRIEFING && brief_tracks.size() == 0)
    req_mode = MENU;

  if (req_mode == INTRO && intro_tracks.size() == 0)
    req_mode = MENU;

  if (req_mode == VICTORY && victory_tracks.size() == 0)
    req_mode = MENU;

  if (req_mode == DEFEAT && defeat_tracks.size() == 0)
    req_mode = MENU;

  if (req_mode == CREDITS && credit_tracks.size() == 0)
    req_mode = MENU;

  if (req_mode == MENU && menu_tracks.size() == 0)
    req_mode = NONE;

  return req_mode;
}

bool MusicDirector::IsNoMusic()
{
  if (music_director)
    return music_director->no_music;

  return true;
}

void MusicDirector::GetNextTrack(int index)
{
  std::vector<std::string>* tracks = nullptr;

  switch (mode)
  {
  case MENU:
    tracks = &menu_tracks;
    break;
  case INTRO:
    tracks = &intro_tracks;
    break;
  case BRIEFING:
    tracks = &brief_tracks;
    break;
  case DEBRIEFING:
    tracks = &debrief_tracks;
    break;
  case PROMOTION:
    tracks = &promote_tracks;
    break;
  case FLIGHT:
    tracks = &flight_tracks;
    break;
  case COMBAT:
    tracks = &combat_tracks;
    break;
  case LAUNCH:
    tracks = &launch_tracks;
    break;
  case RECOVERY:
    tracks = &recovery_tracks;
    break;
  case VICTORY:
    tracks = &victory_tracks;
    break;
  case DEFEAT:
    tracks = &defeat_tracks;
    break;
  case CREDITS:
    tracks = &credit_tracks;
    break;
  default:
    tracks = nullptr;
    break;
  }

  if (tracks && tracks->size())
  {
    if (next_track)
      delete next_track;

    if (index < 0 || index >= tracks->size())
    {
      index = 0;

      if (mode == INTRO)
      {
        mode = MENU;
        ShuffleTracks();
        tracks = &menu_tracks;

        DebugTrace("MusicDirector: INTRO mode complete, switching to MENU\n");

        if (!tracks->size())
          return;
      }
    }

    next_track = NEW MusicTrack(tracks->at(index), mode, index);
    next_track->FadeIn();
  }

  else if (next_track)
    next_track->FadeOut();
}

void MusicDirector::ShuffleTracks()
{
  std::vector<std::string>* tracks = nullptr;

  switch (mode)
  {
  case MENU:
    tracks = &menu_tracks;
    break;
  case INTRO:
    tracks = &intro_tracks;
    break;
  case BRIEFING:
    tracks = &brief_tracks;
    break;
  case DEBRIEFING:
    tracks = &debrief_tracks;
    break;
  case PROMOTION:
    tracks = &promote_tracks;
    break;
  case FLIGHT:
    tracks = &flight_tracks;
    break;
  case COMBAT:
    tracks = &combat_tracks;
    break;
  case LAUNCH:
    tracks = &launch_tracks;
    break;
  case RECOVERY:
    tracks = &recovery_tracks;
    break;
  case VICTORY:
    tracks = &victory_tracks;
    break;
  case DEFEAT:
    tracks = &defeat_tracks;
    break;
  case CREDITS:
    tracks = &credit_tracks;
    break;
  default:
    tracks = nullptr;
    break;
  }

  if (tracks && tracks->size() > 1)
  {
    std::ranges::sort(*tracks);

    std::string t = tracks->at(0);

    if (!isdigit(t[0]))
    {
      std::random_device rd;
      std::mt19937 g(rd());

      std::ranges::shuffle(*tracks, g);
    }
  }
}

DWORD WINAPI MusicDirectorThreadProc(LPVOID link);

void MusicDirector::StartThread()
{
  if (hproc != nullptr)
  {
    DWORD result = 0;
    GetExitCodeThread(hproc, &result);

    if (result != STILL_ACTIVE)
    {
      CloseHandle(hproc);
      hproc = nullptr;
    }
    else
      return;
  }

  if (hproc == nullptr)
  {
    DWORD thread_id = 0;
    hproc = CreateThread(nullptr, 4096, MusicDirectorThreadProc, this, 0, &thread_id);

    if (hproc == nullptr)
    {
      static int report = 10;
      if (report > 0)
      {
        DebugTrace("WARNING: MusicDirector failed to create thread (err=%08x)\n", GetLastError());
        report--;

        if (report == 0)
          DebugTrace("         Further warnings of this type will be supressed.\n");
      }
    }
  }
}

void MusicDirector::StopThread()
{
  if (hproc != nullptr)
  {
    SetMode(SHUTDOWN);
    WaitForSingleObject(hproc, 1500);
    CloseHandle(hproc);
    hproc = nullptr;
  }
}

DWORD WINAPI MusicDirectorThreadProc(LPVOID link)
{
  auto dir = static_cast<MusicDirector*>(link);

  if (dir)
  {
    while (dir->GetMode() != MusicDirector::SHUTDOWN)
    {
      dir->ExecFrame();
      Sleep(100);
    }

    return 0;
  }

  return static_cast<DWORD>(E_POINTER);
}
