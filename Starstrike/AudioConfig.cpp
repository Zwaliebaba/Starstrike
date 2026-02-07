#include "pch.h"
#include "AudioConfig.h"
#include "Button.h"
#include "DataLoader.h"
#include "ParseUtil.h"
#include "Reader.h"

static AudioConfig* audio_config = nullptr;

AudioConfig::AudioConfig()
  : menu_music(90),
    game_music(90),
    efx_volume(90),
    gui_volume(90),
    wrn_volume(90),
    vox_volume(90),
    training(false)
{
  if (!audio_config)
    audio_config = this;
}

AudioConfig::~AudioConfig()
{
  if (audio_config == this)
    audio_config = nullptr;
}

void AudioConfig::Initialize()
{
  audio_config = NEW AudioConfig;
  if (audio_config)
    audio_config->Load();
}

void AudioConfig::Close()
{
  delete audio_config;
  audio_config = nullptr;
}

AudioConfig* AudioConfig::GetInstance() { return audio_config; }

int AudioConfig::MenuMusic()
{
  if (audio_config)
    return -50 * (100 - audio_config->menu_music);

  return 0;
}

int AudioConfig::GameMusic()
{
  int vol = 0;

  if (audio_config)
  {
    vol = -50 * (100 - audio_config->game_music);

    if (audio_config->training)
      vol -= 2000;
  }

  return vol;
}

int AudioConfig::EfxVolume()
{
  int vol = 0;

  if (audio_config)
  {
    vol = -50 * (100 - audio_config->efx_volume);

    if (audio_config->training)
      vol -= 2000;
  }

  return vol;
}

int AudioConfig::GuiVolume()
{
  if (audio_config)
    return -50 * (100 - audio_config->gui_volume);

  return 0;
}

int AudioConfig::WrnVolume()
{
  int vol = 0;

  if (audio_config)
  {
    vol = -50 * (100 - audio_config->wrn_volume);

    if (audio_config->training)
      vol -= 2000;
  }

  return vol;
}

int AudioConfig::VoxVolume()
{
  int vol = 0;

  if (audio_config)
  {
    vol = -50 * (100 - audio_config->vox_volume);

    if (audio_config->training && vol < -750)
      vol = -750;
  }

  return vol;
}

int AudioConfig::Silence() { return -5000; }

void AudioConfig::SetTraining(bool t)
{
  if (audio_config)
    audio_config->training = t;
}

void AudioConfig::SetMenuMusic(int v)
{
  if (v < 0)
    v = 0;
  else
    if (v > 100)
      v = 100;

  menu_music = v;
}

void AudioConfig::SetGameMusic(int v)
{
  if (v < 0)
    v = 0;
  else
    if (v > 100)
      v = 100;

  game_music = v;
}

void AudioConfig::SetEfxVolume(int v)
{
  if (v < 0)
    v = 0;
  else
    if (v > 100)
      v = 100;

  efx_volume = v;
}

void AudioConfig::SetGuiVolume(int v)
{
  if (v < 0)
    v = 0;
  else
    if (v > 100)
      v = 100;

  gui_volume = v;
  Button::SetVolume(-50 * (100 - gui_volume));
}

void AudioConfig::SetWrnVolume(int v)
{
  if (v < 0)
    v = 0;
  else
    if (v > 100)
      v = 100;

  wrn_volume = v;
  Button::SetVolume(-50 * (100 - wrn_volume));
}

void AudioConfig::SetVoxVolume(int v)
{
  if (v < 0)
    v = 0;
  else
    if (v > 100)
      v = 100;

  vox_volume = v;
}

void AudioConfig::Load()
{
  DataLoader* loader = DataLoader::GetLoader();
  auto old_path = loader->GetDataPath();
  loader->SetDataPath();

  // read the config file:
  BYTE* block = nullptr;
  int blocklen = 0;
  auto filename = "audio.cfg";

  FILE* f;
  fopen_s(&f, filename, "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    blocklen = ftell(f);
    fseek(f, 0, SEEK_SET);

    block = NEW BYTE[blocklen + 1];
    block[blocklen] = 0;

    fread(block, blocklen, 1, f);
    fclose(f);
  }

  if (blocklen == 0)
    return;

  Parser parser(NEW BlockReader((const char*)block));
  Term* term = parser.ParseTerm();

  if (!term)
  {
    DebugTrace("ERROR: could not parse '{}'.\n", filename);
    exit(-3);
  }
  TermText* file_type = term->isText();
  if (!file_type || file_type->value() != "AUDIO")
  {
    DebugTrace("WARNING: invalid {} file.  Using defaults\n", filename);
    return;
  }

  do
  {
    delete term;

    term = parser.ParseTerm();

    if (term)
    {
      int v = 0;
      TermDef* def = term->isDef();

      if (def)
      {
        if (def->name()->value() == "menu_music")
        {
          GetDefNumber(v, def, filename);

          if (v < 0 || v > 100)
            DebugTrace("WARNING: Invalid menu_music ({}) in '{}'\n", v, filename);
          else
            menu_music = v;
        }

        else if (def->name()->value() == "game_music")
        {
          GetDefNumber(v, def, filename);

          if (v < 0 || v > 100)
            DebugTrace("WARNING: Invalid game_music ({}) in '{}'\n", v, filename);
          else
            game_music = v;
        }

        else if (def->name()->value() == "efx_volume")
        {
          GetDefNumber(v, def, filename);

          if (v < 0 || v > 100)
            DebugTrace("WARNING: Invalid efx_volume ({}) in '{}'\n", v, filename);
          else
            efx_volume = v;
        }

        else if (def->name()->value() == "gui_volume")
        {
          GetDefNumber(v, def, filename);

          if (v < 0 || v > 100)
            DebugTrace("WARNING: Invalid gui_volume ({}) in '{}'\n", v, filename);
          else
          {
            gui_volume = v;

            Button::SetVolume(-50 * (100 - gui_volume));
          }
        }

        else if (def->name()->value() == "wrn_volume")
        {
          GetDefNumber(v, def, filename);

          if (v < 0 || v > 100)
            DebugTrace("WARNING: Invalid wrn_volume ({}) in '{}'\n", v, filename);
          else
            wrn_volume = v;
        }

        else if (def->name()->value() == "vox_volume")
        {
          GetDefNumber(v, def, filename);

          if (v < 0 || v > 100)
            DebugTrace("WARNING: Invalid vox_volume ({}) in '{}'\n", v, filename);
          else
            vox_volume = v;
        }

        else { DebugTrace("WARNING: unknown label '{}' in '{}'\n", def->name()->value().data(), filename); }
      }
      else
      {
        DebugTrace("WARNING: term ignored in '{}'\n", filename);
        term->print();
        DebugTrace("\n");
      }
    }
  }
  while (term);

  loader->ReleaseBuffer(block);
  loader->SetDataPath(old_path);
}

void AudioConfig::Save()
{
  FILE* f;
  fopen_s(&f, "audio.cfg", "w");
  if (f)
  {
    fprintf(f, "AUDIO\n\n");
    fprintf(f, "menu_music: %3d\n", menu_music);
    fprintf(f, "game_music: %3d\n\n", game_music);
    fprintf(f, "efx_volume: %3d\n", efx_volume);
    fprintf(f, "gui_volume: %3d\n", gui_volume);
    fprintf(f, "wrn_volume: %3d\n", wrn_volume);
    fprintf(f, "vox_volume: %3d\n", vox_volume);
    fclose(f);
  }
}
