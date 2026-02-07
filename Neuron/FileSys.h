#pragma once

#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
constexpr std::wstring_view BaseDirectory{ L"Assets" };
#else
constexpr std::wstring_view BaseDirectory {L"C:\\Users\\zwali\\source\\repos\\DeepSpaceOutpost\\DeepSpaceOutpost\\DeepSpaceOutpost\\Assets\\"};
#endif

namespace Neuron
{
  using byte_buffer_t = Windows::Storage::Streams::IBuffer;

  class FileSys
  {
    public:
      [[nodiscard]] static const hstring& GetSubDirectory() { return sm_directory; }
      [[nodiscard]] static hstring GetSubDirectoryA() { return L"C:\\Users\\zwali\\source\\repos\\DeepSpaceOutpost\\DeepSpaceOutpost.dar\\DeepSpaceOutpost\\Assets"; }
      static void SetSubdirectory(const hstring& _subdir = L"");

    protected:
    inline static hstring sm_directory{ BaseDirectory };

    inline static bool sm_dirChange = { false };
    inline static Windows::Storage::StorageFolder sm_folder = { nullptr };
  };

  class BinaryFile : public FileSys
  {
    public:
      [[nodiscard]] static Windows::Foundation::IAsyncOperation<byte_buffer_t> ReadFileAsync(hstring _fileName);
  };

  class TextFile : public FileSys
  {
    public:
      [[nodiscard]] static Windows::Foundation::IAsyncOperation<hstring> ReadFileAsync(hstring _fileName);
      static Windows::Foundation::IAsyncAction WriteFileAsync(hstring _fileName, const std::wstring& _data);
  };
}
