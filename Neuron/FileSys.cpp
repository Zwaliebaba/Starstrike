#include "pch.h"
#include "FileSys.h"

using namespace Windows::Storage;
using namespace Streams;
using namespace Windows::ApplicationModel;
using namespace Windows::Foundation;

IAsyncOperation<byte_buffer_t> BinaryFile::ReadFileAsync(const hstring _fileName)
{
  try
  {
    if (sm_folder == nullptr)
#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
      sm_folder = { co_await Package::Current().InstalledLocation().GetFolderAsync(sm_directory) };
#else
      sm_folder = {co_await StorageFolder::GetFolderFromPathAsync(sm_directory)};
#endif

    const auto file = co_await sm_folder.GetFileAsync(_fileName.data());
    co_return co_await FileIO::ReadBufferAsync(file);
  }
  catch (const hresult_error& ex)
  {
    Fatal(L"Read File Error {:s} on {:s}", ex.message(), _fileName);
  }
}

IAsyncOperation<hstring> TextFile::ReadFileAsync(const hstring _fileName)
{
  try
  {
    if (sm_folder == nullptr)
#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP))
      sm_folder = { co_await Package::Current().InstalledLocation().GetFolderAsync(m_directory) };
#else
      sm_folder = {co_await StorageFolder::GetFolderFromPathAsync(sm_directory)};
#endif

    const auto file{co_await sm_folder.GetFileAsync(_fileName)};
    co_return co_await FileIO::ReadTextAsync(file);
  }
  catch (const hresult_error& ex)
  {
    Fatal(L"Read File Error {:s} on {:s}", ex.message(), _fileName);
  }
}

IAsyncAction TextFile::WriteFileAsync(hstring _fileName, const std::wstring& _data)
{
  // make a local copy
  const hstring data{_data};

  try
  {
    if (sm_folder == nullptr)
#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
    sm_folder = { co_await Package::Current().InstalledLocation().GetFolderAsync(sm_directory) };
#else
      sm_folder = {co_await StorageFolder::GetFolderFromPathAsync(sm_directory)};
#endif

    const auto file{co_await sm_folder.CreateFileAsync(_fileName, CreationCollisionOption::ReplaceExisting)};
    co_await FileIO::WriteTextAsync(file, data);
  }
  catch (const hresult_error& ex)
  {
    Fatal(L"Write File Error {:s} on {:s}", ex.message(), _fileName);
  }
}

void FileSys::SetSubdirectory(const hstring& _subdir)
{
  if (_subdir.empty())
    sm_directory = BaseDirectory;
  else
    sm_directory = BaseDirectory + L"\\" + _subdir;

  sm_folder = nullptr;
}
