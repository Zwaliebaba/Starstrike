#include "pch.h"
#include "MachineInfo.h"

#define DIRECTINPUT_VERSION 0x0700

#include <ddraw.h>

#pragma comment(lib, "Version.lib")

static int cpu_class = -1;
static int platform = MachineInfo::OS_WINFUTURE;
static int dx_version = -1;
static int total_ram = -1;

static OSVERSIONINFO os_ver = {sizeof(OSVERSIONINFO)};
static SYSTEM_INFO cpu_info;
static MEMORYSTATUS mem_info = {sizeof(MEMORYSTATUS)};

const wchar_t* MachineInfo::GetShortDescription()
{
  static wchar_t desc[256];

  static const wchar_t* cpu_names[] = {
    L"8088",
    L"8086",
    L"80286",
    L"80386",
    L"80486",
    L"Pentium",
    L"Pentium II",
    L"Pentium 3",
    L"Pentium 4"
  };

  static const wchar_t* os_names[] = {
    L"DOS",
    L"Windows 95",
    L"Windows 98",
    L"Windows NT",
    L"Windows 2000",
    L"Windows XP",
    L"Windows XP x64",
    L"Windows Vista",
    L"Windows Seven",
    L"Future Windows"
  };

  int cpu_index = GetCpuClass();
  if (cpu_index < 0)
    cpu_index = 0;
  else if (cpu_index > 8)
    cpu_index = 8;

  int os_index = GetPlatform();
  os_index = std::max(os_index, 0);

  swprintf_s(desc, L"%s %d MB RAM %s", cpu_names[cpu_index], GetTotalRam(), os_names[os_index]);

  return desc;
}

/****************************************************************************
*
*      GetDXVersion
*
*  This function returns
*              0       Insufficient DirectX installed
*              9       At least DirectX 9 installed.
*
****************************************************************************/

DWORD GetDXVersion() { return 9; }

int MachineInfo::GetCpuClass()
{
  if (cpu_class < 0)
  {
    GetSystemInfo(&cpu_info);

    if (cpu_info.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL || cpu_info.dwProcessorType <
      PROCESSOR_INTEL_PENTIUM)
      DebugTrace(L"INCOMPATIBLE CPU TYPE!\n");

    if (GetPlatform() < OS_WINNT)
      cpu_class = CPU_P5;

    else
      cpu_class = cpu_info.wProcessorLevel;
  }

  return cpu_class;
}

int MachineInfo::GetTotalRam()
{
  if (total_ram < 0)
  {
    GlobalMemoryStatus(&mem_info);
    total_ram = static_cast<int>(mem_info.dwTotalPhys / (1024 * 1024)) + 1;
  }

  return total_ram;
}

int MachineInfo::GetPlatform() { return platform; }

int MachineInfo::GetDirectXVersion()
{
  if (dx_version < 0)
    dx_version = GetDXVersion();

  return dx_version;
}
