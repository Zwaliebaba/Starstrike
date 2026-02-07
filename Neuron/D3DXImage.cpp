#include "pch.h"
#include "D3DXImage.h"
#include "VideoDX9.h"

D3DXImage::D3DXImage()
  : image(nullptr),
    width(0),
    height(0),
    format(0) {}

D3DXImage::D3DXImage(WORD w, WORD h, DWORD* img)
{
  ZeroMemory(this, sizeof(D3DXImage));

  width = w;
  height = h;

  int pixels = width * height;

  image = NEW DWORD [pixels];

  if (image && pixels)
  {
    for (int i = 0; i < pixels; i++)
      image[i] = img[i];
  }
}

D3DXImage::~D3DXImage() { delete [] image; }

bool D3DXImage::Load(const char* filename)
{
  bool success = false;
  FILE* f;

  fopen_s(&f, filename, "rb");
  if (f == nullptr)
    return success;

  int len = 0;
  BYTE* buf = nullptr;

  fseek(f, 0, SEEK_END);
  len = ftell(f);
  fseek(f, 0, SEEK_SET);

  buf = NEW BYTE[len];

  if (buf)
  {
    fread(buf, len, 1, f);
    fclose(f);

    success = LoadBuffer(buf, len);
  }

  return success;
}

bool D3DXImage::LoadBuffer(BYTE* buf, int len)
{
  bool success = false;
  HRESULT hr = E_FAIL;
  D3DXIMAGE_INFO info;

  if (buf == nullptr)
    return success;

  hr = D3DXGetImageInfoFromFileInMemory(buf, len, &info);

  if (FAILED(hr))
    return success;

  width = info.Width;
  height = info.Height;
  format = info.Format;

  if (image)
  {
    delete [] image;
    image = nullptr;
  }

  IDirect3DSurface9* surf = nullptr;
  IDirect3DDevice9* dev = VideoDX9::GetD3DDevice9();

  hr = dev->CreateOffscreenPlainSurface(width, height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, nullptr);

  if (FAILED(hr))
    return success;

  hr = D3DXLoadSurfaceFromFileInMemory(surf,             // dest surface
                                       nullptr,             // dest palette (none)
                                       nullptr,             // dest rect (entire image)
                                       buf,              // memory file
                                       len,              // size of data
                                       nullptr,             // source rect (entire image)
                                       D3DX_DEFAULT,     // filter operation
                                       0,                // color key (none)
                                       nullptr);            // image info

  if (SUCCEEDED(hr))
  {
    D3DLOCKED_RECT locked_rect;
    hr = surf->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY);

    if (SUCCEEDED(hr))
    {
      // copy surface into image
      image = NEW DWORD[width * height];
      if (image)
      {
        for (DWORD i = 0; i < height; i++)
        {
          auto dst = (BYTE*)(image + i * width);
          BYTE* src = static_cast<BYTE*>(locked_rect.pBits) + i * locked_rect.Pitch;

          CopyMemory(dst, src, width * sizeof(DWORD));
        }

        success = true;
      }

      surf->UnlockRect();
    }
  }

  surf->Release();
  surf = nullptr;

  return success;
}

bool D3DXImage::Save(char* filename)
{
  bool success = false;
  HRESULT hr = E_FAIL;

  if (!image || !width || !height)
    return success;

  FILE* f;
  fopen_s(&f, filename, "wb");
  if (f == nullptr)
    return success;

  IDirect3DSurface9* surf = nullptr;
  IDirect3DDevice9* dev = VideoDX9::GetD3DDevice9();

  hr = dev->CreateOffscreenPlainSurface(width, height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, nullptr);

  if (SUCCEEDED(hr))
  {
    D3DLOCKED_RECT locked_rect;
    hr = surf->LockRect(&locked_rect, nullptr, 0);

    if (SUCCEEDED(hr))
    {
      // copy image into surface
      for (DWORD i = 0; i < height; i++)
      {
        auto src = (BYTE*)(image + i * width);
        BYTE* dst = static_cast<BYTE*>(locked_rect.pBits) + i * locked_rect.Pitch;

        CopyMemory(dst, src, width * sizeof(DWORD));
      }

      surf->UnlockRect();

      ID3DXBuffer* buffer = nullptr;
      D3DXIMAGE_FILEFORMAT imgfmt = D3DXIFF_PNG;

      if (strstr(filename, ".jpg") || strstr(filename, ".JPG"))
        imgfmt = D3DXIFF_JPG;

      else if (strstr(filename, ".bmp") || strstr(filename, ".BMP"))
        imgfmt = D3DXIFF_BMP;

      hr = D3DXSaveSurfaceToFileInMemory(&buffer,  // destination
                                         imgfmt,  // type of file
                                         surf,    // image to save
                                         nullptr,    // palette
                                         nullptr);   // source rect (entire image)

      if (SUCCEEDED(hr))
      {
        fwrite(buffer->GetBufferPointer(), buffer->GetBufferSize(), 1, f);
        success = true;
      }
    }
  }

  surf->Release();
  surf = nullptr;
  fclose(f);
  return success;
}
