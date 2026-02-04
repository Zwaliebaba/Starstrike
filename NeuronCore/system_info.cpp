#include "pch.h"
#include <mmsystem.h>

#include "system_info.h"

SystemInfo* g_systemInfo = nullptr;

SystemInfo::SystemInfo()
{
  GetLocaleDetails();
  GetAudioDetails();
  GetDirectXVersion();
}

SystemInfo::~SystemInfo() {}

void SystemInfo::GetLocaleDetails()
{
  int size;
  bool languageSuccess = false;

  if (!languageSuccess)
  {
    size = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, nullptr, 0);
    m_localeInfo.m_language = new char[size + 1];
    ASSERT_TEXT(GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, m_localeInfo.m_language, size),
                "Couldn't get locale details");
  }

  size = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SENGCOUNTRY, nullptr, 0);
  m_localeInfo.m_country = new char[size + 1];
  ASSERT_TEXT(GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SENGCOUNTRY, m_localeInfo.m_country, size),
              "Couldn't get country details");
}

void SystemInfo::GetAudioDetails()
{
  unsigned int numDevs = waveOutGetNumDevs();
  m_audioInfo.m_numDevices = numDevs;
  m_audioInfo.m_preferredDevice = -1;
  int bestScore = -1000;

  m_audioInfo.m_deviceNames = new char*[numDevs];
  for (unsigned int i = 0; i < numDevs; ++i)
  {
    WAVEOUTCAPSA caps;
    waveOutGetDevCapsA(i, &caps, sizeof(WAVEOUTCAPSA));
    m_audioInfo.m_deviceNames[i] = _strdup(caps.szPname);

    int score = 0;
    if (caps.dwFormats & WAVE_FORMAT_1M08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_1M16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_1S08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_1S16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2M08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2M16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2S08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_2S16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4M08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4M16)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4S08)
      score++;
    if (caps.dwFormats & WAVE_FORMAT_4S16)
      score++;
    if (caps.dwSupport & WAVECAPS_SYNC)
      score -= 10;
    if (caps.dwSupport & WAVECAPS_LRVOLUME)
      score++;
    if (caps.dwSupport & WAVECAPS_PITCH)
      score++;
    if (caps.dwSupport & WAVECAPS_PLAYBACKRATE)
      score++;
    if (caps.dwSupport & WAVECAPS_VOLUME)
      score++;
    if (caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
      score++;

    if (score > bestScore)
      m_audioInfo.m_preferredDevice = i;
  }

  ASSERT_TEXT(m_audioInfo.m_preferredDevice != -1, "No suitable audio hardware found");
}

void SystemInfo::GetDirectXVersion()
{
  HKEY hkey;
  long errCode;
  unsigned long bufLen = 256;
  unsigned char buf[256];

  m_directXVersion = -1;

  errCode = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\DirectX", 0, KEY_READ, &hkey);
  ASSERT_TEXT(errCode == ERROR_SUCCESS, "Failed to get DirectX Version");
  errCode = RegQueryValueExA(hkey, "InstalledVersion", nullptr, nullptr, buf, &bufLen);

  if (errCode == ERROR_SUCCESS)
    m_directXVersion = buf[3];
  else
  {
    // NOTE by Chris : This value doesn't exist on Windows98
    // However the key "Version" does exist
    errCode = RegQueryValueExA(hkey, "Version", nullptr, nullptr, buf, &bufLen);
    if (errCode == ERROR_SUCCESS)
      m_directXVersion = buf[3];
  }

  RegCloseKey(hkey);
}
