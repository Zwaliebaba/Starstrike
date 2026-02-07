#ifndef D3DXImage_h
#define D3DXImage_h

struct D3DXImage
{
  
  D3DXImage();
  D3DXImage(WORD w, WORD h, DWORD* img);
  ~D3DXImage();

  bool Load(const char* filename);
  bool Save(char* filename);

  bool LoadBuffer(BYTE* buf, int len);

  DWORD* image;
  DWORD width;
  DWORD height;
  DWORD format;
};

#endif D3DXImage_h
