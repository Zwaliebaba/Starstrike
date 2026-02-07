#ifndef DataLoader_h
#define DataLoader_h

#include "List.h"


#undef LoadBitmap

class Bitmap;
class Sound;
class Video;

class DataLoader
{
  public:
    
    enum
    {
      DATAFILE_OK,
      DATAFILE_INVALID,
      DATAFILE_NOTEXIST
    };

    DataLoader();

    static DataLoader* GetLoader() { return loader; }
    static void Initialize();
    static void Close();

    void Reset();
    void UseVideo(Video* v);
    void EnableMedia(bool enable = true);

    void SetDataPath(std::string_view path = {});
    std::string GetDataPath() const;

    bool IsMediaLoadEnabled() const { return enable_media; }

    bool FindFile(std::string_view fname);
    int ListFiles(std::string_view filter, std::vector<std::string>& list, bool recurse = false);
    int LoadBuffer(std::string_view name, BYTE*& buf, bool null_terminate = false, bool optional = false);
    int LoadBitmap(std::string_view _name, Bitmap& bmp, int type = 0, bool optional = false);
    int CacheBitmap(std::string_view name, Bitmap*& bmp, int type = 0, bool optional = false);
    int LoadTexture(std::string_view _name, Bitmap*& bmp, int type = 0, bool preload_cache = false, bool optional = false);
    int LoadSound(std::string_view fname, Sound*& snd, DWORD flags = 0, bool optional = false);
    int LoadStream(std::string_view fname, Sound*& snd, bool optional = false);

    void ReleaseBuffer(BYTE*& buf);
    int fread(void* buffer, size_t size, size_t count, BYTE*& stream);

  private:
    int LoadIndexed(std::string_view name, Bitmap& bmp, int type);
    int LoadHiColor(std::string_view name, Bitmap& bmp, int type);
    int LoadAlpha(std::string_view name, Bitmap& bmp);

    void ListFileSystem(std::string_view filter, std::vector<std::string>& list, std::string base_path, bool recurse);
    int LoadPartialFile(std::string_view fname, BYTE*& buf, int max_load, bool optional = false);

    std::string datapath;
    Video* video;
    bool enable_media;

    static DataLoader* loader;
};

#endif DataLoader_h
