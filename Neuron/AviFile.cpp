#include "pch.h"
#include "AviFile.h"
#include <vfw.h>

#pragma comment(lib, "vfw32.lib")

AviFile::AviFile(const char* fname)
  : filename(fname),
    fps(0),
    pfile(nullptr),
    ps(nullptr),
    ps_comp(nullptr),
    frame_size(0),
    nframe(0),
    nsamp(0),
    play(true),
    iserr(false) { AVIFileInit(); }

AviFile::AviFile(const char* fname, const Rect& r, int frame_rate)
  : rect(r),
    filename(fname),
    fps(frame_rate),
    pfile(nullptr),
    ps(nullptr),
    ps_comp(nullptr),
    nframe(0),
    nsamp(0),
    play(false),
    iserr(false)
{
  DebugTrace("\n\nAviFile({}, w=%d, h=%d, f=%d FPS)\n", fname, r.w, r.h, fps);
  frame_size = r.w * r.h * 3;

  AVIFileInit();
  HRESULT hr = AVIFileOpenA(&pfile, fname, OF_WRITE | OF_CREATE, nullptr);

  if (hr != AVIERR_OK)
  {
    DebugTrace("AviFile - open failed. %08x\n", hr);
    iserr = true;
    return;
  }

  DebugTrace("AviFile - open successful\n");

  AVISTREAMINFO strhdr;
  ZeroMemory(&strhdr, sizeof(strhdr));

  strhdr.fccType = streamtypeVIDEO;
  strhdr.fccHandler = 0;
  strhdr.dwScale = 1000 / fps;
  strhdr.dwRate = 1000;
  strhdr.dwSuggestedBufferSize = frame_size;

  SetRect(&strhdr.rcFrame, 0, 0, r.w, r.h);

  hr = AVIFileCreateStream(pfile, &ps, &strhdr);

  if (hr != AVIERR_OK)
  {
    DebugTrace("AviFile - create stream failed. %08x\n", hr);
    iserr = true;
    return;
  }

  DebugTrace("AviFile - create stream successful\n");

  AVICOMPRESSOPTIONS opts;
  ZeroMemory(&opts, sizeof(opts));
  opts.fccType = streamtypeVIDEO;
  //opts.fccHandler = mmioFOURCC('W','M','V','3');
  opts.fccHandler = mmioFOURCC('D', 'I', 'B', ' ');  // (full frames)
  opts.dwFlags = 8;

  hr = AVIMakeCompressedStream(&ps_comp, ps, &opts, nullptr);
  if (hr != AVIERR_OK)
  {
    DebugTrace("AviFile - compressed stream failed. %08x\n", hr);
    iserr = true;
    return;
  }

  DebugTrace("AviFile - make compressed stream successful\n");

  BITMAPINFOHEADER bmih;
  ZeroMemory(&bmih, sizeof(BITMAPINFOHEADER));

  bmih.biSize = sizeof(BITMAPINFOHEADER);
  bmih.biBitCount = 24;
  bmih.biCompression = BI_RGB;
  bmih.biWidth = rect.w;
  bmih.biHeight = rect.h;
  bmih.biPlanes = 1;
  bmih.biSizeImage = frame_size;

  hr = AVIStreamSetFormat(ps_comp, 0, &bmih, sizeof(BITMAPINFOHEADER));

  if (hr != AVIERR_OK)
  {
    DebugTrace("AviFile - stream format failed. %08x\n", hr);
    iserr = true;
    return;
  }

  DebugTrace("AviFile - stream format successful\n");
}

AviFile::~AviFile()
{
  if (!play)
    DebugTrace("*** Closing AVI file '{}' with %d frames\n", filename, nframe);

  if (ps_comp)
    AVIStreamRelease(ps_comp);
  if (ps)
    AVIStreamRelease(ps);
  if (pfile)
    AVIFileRelease(pfile);

  AVIFileExit();
}

//
// Note that AVI frames use DIB format - Y is inverted.
// So we need to flip the native bmp before sending to the
// file.

HRESULT AviFile::AddFrame(const Bitmap& bmp)
{
  HRESULT hr = E_FAIL;

  if (!iserr && !play && bmp.IsHighColor() && bmp.Width() == rect.w && bmp.Height() == rect.h)
  {
    int w = bmp.Width();
    int h = bmp.Height();
    auto buffer = NEW BYTE[frame_size];
    BYTE* dst = buffer;

    for (int y = 0; y < bmp.Height(); y++)
    {
      Color* src = bmp.HiPixels() + (h - 1 - y) * w;

      for (int x = 0; x < bmp.Width(); x++)
      {
        *dst++ = static_cast<BYTE>(src->Blue());
        *dst++ = static_cast<BYTE>(src->Green());
        *dst++ = static_cast<BYTE>(src->Red());
        src++;
      }
    }

#pragma warning(suppress: 6001)
    hr = AVIStreamWrite(ps_comp, nframe, 1, buffer, frame_size, AVIIF_KEYFRAME, nullptr, nullptr);

    if (SUCCEEDED(hr))
      nframe++;
    else
    {
      DebugTrace("AVIStreamWriteFile failed. %08x\n", hr);
      iserr = true;
    }

    delete [] buffer;
  }

  return hr;
}

HRESULT AviFile::GetFrame(double seconds, Bitmap& bmp)
{
  HRESULT hr = E_FAIL;
  return hr;
}
