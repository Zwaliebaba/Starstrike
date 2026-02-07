#include "pch.h"
#include "DataLoader.h"
#include "Bitmap.h"
#include "D3DXImage.h"
#include "PCX.h"
#include "Sound.h"
#include "Video.h"
#include "Wave.h"

static DataLoader* g_defLoader = nullptr;
DataLoader* DataLoader::loader = nullptr;
static std::string g_defaultPath =
  R"(C:\Users\zwali\source\repos\StarShatter\InterstellarOutpost\x64\Debug\InterstellarOutpost\AppX\Assets\)";

DataLoader::DataLoader()
  : datapath(g_defaultPath),
    video(nullptr),
    enable_media(true) {}

void DataLoader::UseVideo(Video* v) { video = v; }

void DataLoader::Initialize()
{
  g_defLoader = NEW DataLoader;
  loader = g_defLoader;
}

void DataLoader::Close()
{
  Bitmap::ClearCache();

  delete g_defLoader;
  g_defLoader = nullptr;
  loader = nullptr;
}

void DataLoader::Reset() { Close(); }

void DataLoader::EnableMedia(bool enable) { enable_media = enable; }

void DataLoader::SetDataPath(std::string_view path) { datapath = g_defaultPath + std::string(path); }

std::string DataLoader::GetDataPath() const { return datapath.substr(g_defaultPath.size()); }

bool DataLoader::FindFile(std::string_view name)
{
  // assemble file name:
  std::string filename = datapath + std::string(name);
  std::ranges::replace(filename, '/', '\\');

  // first check current directory:
  FILE* f;
  fopen_s(&f, filename.c_str(), "rb");

  if (f)
  {
    fclose(f);
    return true;
  }

  return false;
}

int DataLoader::ListFiles(std::string_view filter, std::vector<std::string>& list, bool recurse)
{
  list.clear();

  ListFileSystem(filter, list, datapath.c_str(), recurse);

  return list.size();
}

void DataLoader::ListFileSystem(std::string_view filter, std::vector<std::string>& list, std::string base_path, bool recurse)
{
  char data_filter[256];
  ZeroMemory(data_filter, 256);

  auto pf = filter.begin();
  char* pdf = data_filter;

  while (pf != filter.end())
  {
    if (*pf != '*')
      *pdf++ = *pf;
    ++pf;
  }

  int pathlen = base_path.length();

  // assemble win32 find filter:
  char win32_filter[1024];
  strcpy_s(win32_filter, datapath.c_str());

  if (recurse)
    strcat_s(win32_filter, "*.*");
  else
    strcat_s(win32_filter, filter.data());

  // first check current directory:
  WIN32_FIND_DATAA data;
  HANDLE hFind = FindFirstFileA(win32_filter, &data);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
      {
        if (recurse && data.cFileName[0] != '.')
        {
          std::string old_datapath = datapath;

          std::string newpath = datapath;
          newpath += data.cFileName;
          newpath += "\\";
          datapath = newpath;

          ListFileSystem(filter, list, base_path, recurse);

          datapath = old_datapath;
        }
      }
      else
      {
        if (filter == "*.*" || strstr(data.cFileName, data_filter))
        {
          std::string full_name = datapath;
          full_name += data.cFileName;

          list.emplace_back(full_name.substr(pathlen, 1000));
        }
      }
    }
    while (FindNextFileA(hFind, &data));
  }
}

int DataLoader::LoadBuffer(std::string_view name, BYTE*& buf, bool null_terminate, bool optional)
{
  buf = nullptr;

  // assemble file name:
  std::string filename = datapath + std::string(name);
  std::ranges::replace(filename, '/', '\\');

  // first check current directory:
  FILE* f;
  fopen_s(&f, filename.c_str(), "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (null_terminate)
    {
      buf = NEW BYTE[len + 1];
      if (buf)
        buf[len] = 0;
    }

    else
      buf = NEW BYTE[len];

    if (buf)
      ::fread(buf, len, 1, f);

    fclose(f);

    return len;
  }

  if (!optional)
    DebugTrace("WARNING - DataLoader could not load buffer '{}'\n", filename);
  return 0;
}

int DataLoader::LoadPartialFile(std::string_view name, BYTE*& buf, int max_load, bool optional)
{
  buf = nullptr;

  // assemble file name:
  std::string filename = datapath + std::string(name);
  std::ranges::replace(filename, '/', '\\');

  // first check current directory:
  FILE* f;
  fopen_s(&f, filename.c_str(), "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);

    len = std::min(len, max_load);

    buf = NEW BYTE[len];

    if (buf)
      ::fread(buf, len, 1, f);

    fclose(f);

    return len;
  }

  if (!optional)
    DebugTrace("WARNING - DataLoader could not load partial file '{}'\n", filename);
  return 0;
}

int DataLoader::fread(void* buffer, size_t size, size_t count, BYTE*& stream)
{
  CopyMemory(buffer, stream, size*count);
  stream += size * count;

  return size * count;
}

void DataLoader::ReleaseBuffer(BYTE*& buf)
{
  delete [] buf;
  buf = nullptr;
}

int DataLoader::CacheBitmap(std::string_view name, Bitmap*& bitmap, int type, bool optional)
{
  int result = 0;

  // search cache:
  bitmap = Bitmap::CheckCache(name);
  if (bitmap)
    return 1;

  // not in cache yet:
  bitmap = NEW Bitmap;

  if (bitmap)
    result = LoadBitmap(name, *bitmap, type, optional);

  if (result && bitmap)
    Bitmap::AddToCache(bitmap);

  return result;
}

int DataLoader::LoadBitmap(std::string_view _name, Bitmap& bitmap, int type, bool optional)
{
  std::string name(_name);
  std::ranges::replace(name, '/', '\\');

  if (!enable_media)
    return 0;

  int result = LoadIndexed(_name, bitmap, type);

  // check for a matching high color bitmap:
  if (result == 1)
  {
    int hi_result = LoadHiColor(_name, bitmap, type);

    if (hi_result == 2)
      result = 3;
  }

  bitmap.SetFilename(_name);

  if (!result && !optional)
    DebugTrace("WARNING - DataLoader could not load bitmap '{}{}'\n", datapath.data(), _name);

  return result;
}

int DataLoader::LoadTexture(std::string_view _name, Bitmap*& bitmap, int type, bool preload_cache, bool optional)
{
  if (!enable_media)
    return 0;

  int result = 0;

  // assemble file name:
  std::string filename(_name);
  std::ranges::replace(filename, '/', '\\');

  // search cache:
  bitmap = Bitmap::CheckCache(filename.c_str());
  if (bitmap)
    return 1;

  // not in cache yet:
  bitmap = NEW Bitmap;

  if (bitmap)
  {
    result = LoadHiColor(filename, *bitmap, type);

    if (!result)
      result = LoadIndexed(filename, *bitmap, type);

    bitmap->SetFilename(filename);

    if (result)
    {
      bitmap->MakeTexture();
      Bitmap::AddToCache(bitmap);
    }
    else
    {
      delete bitmap;
      bitmap = nullptr;

      if (!optional)
        DebugTrace("WARNING - DataLoader could not load texture '{}'\n", filename.c_str());
    }
  }
  else if (!optional)
    DebugTrace("WARNING - DataLoader could not allocate texture '{}'\n", filename.c_str());

  return result;
}

int DataLoader::LoadIndexed(std::string_view name, Bitmap& bitmap, int type)
{
  if (!enable_media)
    return 0;

  int result = 0;
  PcxImage pcx;
  D3DXImage d3dx;
  bool pcx_file = name.contains(".pcx") || name.contains(".PCX");

  // assemble file name:
  std::string filename = datapath + std::string(name);

  if (pcx_file)
    pcx.Load(filename.c_str());

  else
    d3dx.Load(filename.c_str());

  // now copy the image into the bitmap:
  if (pcx_file)
  {
    if (pcx.bitmap)
    {
      bitmap.CopyImage(pcx.width, pcx.height, pcx.bitmap, type);
      result = 1;
    }

    else if (pcx.himap)
    {
      bitmap.CopyHighColorImage(pcx.width, pcx.height, pcx.himap, type);
      result = 2;
    }

    if (result == 2)
      LoadAlpha(name, bitmap);
  }

  else
  {
    if (d3dx.image)
    {
      bitmap.CopyHighColorImage(d3dx.width, d3dx.height, d3dx.image, type);
      result = 2;
    }

    if (result == 2)
      LoadAlpha(name, bitmap);
  }

  return result;
}

int DataLoader::LoadHiColor(std::string_view name, Bitmap& bitmap, int type)
{
  if (!enable_media)
    return 0;

  int result = 0;
  PcxImage pcx;
  D3DXImage d3dx;
  bool pcx_file = name.contains(".pcx") || name.contains(".PCX");
  bool bmp_file = name.contains(".bmp") || name.contains(".BMP");
  bool png_file = name.contains(".png") || name.contains(".PNG");

  // check for a matching high color bitmap:
  char name2[256];
  strcpy_s(name2, name.data());

  char* dot = strrchr(name2, '.');
  if (dot && pcx_file)
    strcpy(dot, "+.pcx");
  else if (dot && bmp_file)
    strcpy(dot, "+.bmp");
  else if (dot && png_file)
    strcpy(dot, "+.png");
  else
    return result;

  std::string filename = datapath + name2;

  if (pcx_file)
    pcx.Load(filename.c_str());

  else
    d3dx.Load(filename.c_str());

  // now copy the image into the bitmap:
  if (pcx_file && pcx.himap)
  {
    bitmap.CopyHighColorImage(pcx.width, pcx.height, pcx.himap, type);
    result = 2;
  }

  else if (d3dx.image)
  {
    bitmap.CopyHighColorImage(d3dx.width, d3dx.height, d3dx.image, type);
    result = 2;
  }

  if (result == 2)
    LoadAlpha(name, bitmap);

  return result;
}

int DataLoader::LoadAlpha(std::string_view name, Bitmap& bitmap)
{
  if (!enable_media)
    return 0;

  PcxImage pcx;
  D3DXImage d3dx;
  bool pcx_file = name.contains(".pcx") || name.contains(".PCX");
  bool bmp_file = name.contains(".bmp") || name.contains(".BMP");
  bool png_file = name.contains(".png") || name.contains(".PNG");
  bool tga_file = name.contains(".tga") || name.contains(".TGA");

  // check for an associated alpha-only (grayscale) bitmap:
  char name2[256];
  strcpy_s(name2, name.data());
  char* dot = strrchr(name2, '.');
  if (dot && pcx_file)
    strcpy(dot, "@.pcx");
  else if (dot && bmp_file)
    strcpy(dot, "@.bmp");
  else if (dot && png_file)
    strcpy(dot, "@.png");
  else if (dot && tga_file)
    strcpy(dot, "@.tga");
  else
    return 0;

  std::string filename = datapath + name2;

  if (pcx_file)
    pcx.Load(filename.c_str());
  else
    d3dx.Load(filename.c_str());

  // now copy the alpha values into the bitmap:
  if (pcx_file && pcx.bitmap)
    bitmap.CopyAlphaImage(pcx.width, pcx.height, pcx.bitmap);
  else if (pcx_file && pcx.himap)
    bitmap.CopyAlphaRedChannel(pcx.width, pcx.height, pcx.himap);
  else if (d3dx.image)
    bitmap.CopyAlphaRedChannel(d3dx.width, d3dx.height, d3dx.image);

  return 0;
}

int DataLoader::LoadSound(std::string_view name, Sound*& snd, DWORD flags, bool optional)
{
  if (!enable_media)
    return 0;

  int result = 0;

  WAVE_HEADER head;
  WAVE_FMT fmt;
  WAVE_FACT fact;
  WAVE_DATA data;
  WAVEFORMATEX wfex;
  LPBYTE wave;

  LPBYTE buf;
  LPBYTE p;

  ZeroMemory(&head, sizeof(head));
  ZeroMemory(&fmt, sizeof(fmt));
  ZeroMemory(&fact, sizeof(fact));
  ZeroMemory(&data, sizeof(data));

  int len = LoadBuffer(name, buf, false, optional);

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
            ReleaseBuffer(buf);
            return result;
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
      wave = p;

      snd = Sound::Create(flags, &wfex, data.chunk_size, wave);

      if (snd)
        result = data.chunk_size;
    }
  }

  ReleaseBuffer(buf);
  return result;
}

int DataLoader::LoadStream(std::string_view name, Sound*& snd, bool optional)
{
  if (!enable_media)
    return 0;

  if (name.empty())
    return 0;

  size_t namelen = name.size();

  if (namelen < 5)
    return 0;

  int result = 0;

  WAVE_HEADER head;
  WAVE_FMT fmt;
  WAVE_FACT fact;
  WAVE_DATA data;
  WAVEFORMATEX wfex;

  LPBYTE buf;
  LPBYTE p;
  int len;

  ZeroMemory(&head, sizeof(head));
  ZeroMemory(&fmt, sizeof(fmt));
  ZeroMemory(&fact, sizeof(fact));
  ZeroMemory(&data, sizeof(data));

  len = LoadPartialFile(name, buf, 4096, optional);

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
            ReleaseBuffer(buf);
            return result;
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

      snd = Sound::Create(Sound::STREAMED, &wfex);

      if (snd)
      {
        // assemble file name:
        std::string filename = datapath + std::string(name);
        snd->StreamFile(filename.c_str(), p - buf);

        result = data.chunk_size;
      }
    }
  }

  ReleaseBuffer(buf);
  return result;
}
