#include "pch.h"
#include "Intel.h"

namespace
{
  constexpr std::string_view INTEL_NAME[] = {"", "Reserve", "Secret", "Known", "Located", "Tracked",};
}

std::string Intel::NameFromIntel(int intel) { return std::string(INTEL_NAME[intel]); }

int Intel::IntelFromName(std::string_view type_name)
{
  for (int i = RESERVE; i <= TRACKED; i++)
  {
    if (type_name == INTEL_NAME[i])
      return i;
  }

  return 0;
}
