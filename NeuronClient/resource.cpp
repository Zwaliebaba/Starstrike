#include "pch.h"
#include "resource.h"

#include "FileSys.h"
#include "GameApp.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "filesys_utils.h"
#include "location.h"
#include "preferences.h"
#include "renderer.h"
#include "shape.h"
#include "sound_stream_decoder.h"
#include "text_stream_readers.h"

void Resource::Shutdown()
{
  FlushOpenGlState();
  m_bitmaps.EmptyAndDelete();
  m_shapes.EmptyAndDelete();
}

void Resource::AddBitmap(const char* _name, const BitmapRGBA& _bmp, bool _mipMapping)
{
  // Only insert if a bitmap with no other bitmap is already using that name
  if (m_bitmaps.GetIndex(_name) == -1)
  {
    auto bmpCopy = new BitmapRGBA(_bmp);
    m_bitmaps.PutData(_name, bmpCopy);
  }
}

void Resource::DeleteBitmap(const char* _name)
{
  int index = m_bitmaps.GetIndex(_name);
  if (index >= 0)
  {
    BitmapRGBA* bmp = m_bitmaps.GetData(index);
    delete bmp;
    m_bitmaps.RemoveData(index);
  }
}

const BitmapRGBA* Resource::GetBitmap(const char* _name) { return m_bitmaps.GetData(_name); }

TextReader* Resource::GetTextReader(std::string_view _filename)
{
  TextReader* reader = nullptr;
  hstring fullFilename = FileSys::GetHomeDirectory() + to_hstring(_filename);

  if (DoesFileExist(to_string(fullFilename).c_str()))
    reader = new TextFileReader(to_string(fullFilename).c_str());

  return reader;
}

BinaryReader* Resource::GetBinaryReader(const char* _filename)
{
  BinaryReader* reader = nullptr;
  hstring fullFilename = FileSys::GetHomeDirectory() + to_hstring(_filename);

  if (DoesFileExist(to_string(fullFilename).c_str()))
    reader = new BinaryFileReader(to_string(fullFilename).c_str());

  return reader;
}

int Resource::GetTexture(const char* _name, bool _mipMapping, bool _masked)
{
  // First lookup this name in the BTree of existing textures
  int theTexture = m_textures.GetData(_name, -1);

  // If the texture wasn't there, then look in our bitmap store
  if (theTexture == -1)
  {
    BitmapRGBA* bmp = m_bitmaps.GetData(_name);
    if (bmp)
    {
      if (_masked)
        bmp->ConvertPinkToTransparent();
      theTexture = bmp->ConvertToTexture(_mipMapping);
      m_textures.PutData(_name, theTexture);
    }
  }

  // If we still didn't find it, try to load it from a file on the disk
  if (theTexture == -1)
  {
    BinaryReader* reader = GetBinaryReader(_name);

    if (reader)
    {
      const char* extension = GetExtensionPart(_name);
      BitmapRGBA bmp(reader, extension);
      delete reader;

      if (_masked)
        bmp.ConvertPinkToTransparent();
      theTexture = bmp.ConvertToTexture(_mipMapping);
      m_textures.PutData(_name, theTexture);
    }
  }

  if (theTexture == -1)
    Fatal("Failed to load texture {}", _name);

  return theTexture;
}

bool Resource::DoesTextureExist(const char* _name)
{
  // First lookup this name in the BTree of existing textures
  int theTexture = m_textures.GetData(_name, -1);
  if (theTexture != -1)
    return true;

  // If the texture wasn't there, then look in our bitmap store
  BitmapRGBA* bmp = m_bitmaps.GetData(_name);
  if (bmp)
    return true;

  // If we still didn't find it, try to load it from a file on the disk
  char fullPath[512];
  sprintf(fullPath, "%s", _name);
  _strlwr(fullPath);
  BinaryReader* reader = GetBinaryReader(fullPath);
  if (reader)
    return true;

  return false;
}

void Resource::DeleteTexture(const char* _name)
{
  int id = m_textures.GetData(_name);
  if (id > 0)
  {
    m_textures.RemoveData(_name);
  }
}

Shape* Resource::GetShape(const char* _name)
{
  Shape* theShape = m_shapes.GetData(_name);

  // If we haven't loaded the shape before, or _makeNew is true, then
  // try to load it from the disk
  if (!theShape)
  {
    theShape = GetShapeCopy(_name, false);
    m_shapes.PutData(_name, theShape);
  }

  return theShape;
}

Shape* Resource::GetShapeCopy(const char* _name, bool _animating)
{
  Shape* theShape = nullptr;

  hstring fullFilename = FileSys::GetHomeDirectory() + L"shapes\\" + to_hstring(_name);

  if (DoesFileExist(to_string(fullFilename).c_str()))
    theShape = new Shape(to_string(fullFilename).c_str(), _animating);

  ASSERT_TEXT(theShape, "Couldn't create shape file {}", _name);
  return theShape;
}

SoundStreamDecoder* Resource::GetSoundStreamDecoder(const char* _filename)
{
  char buf[256];
  sprintf(buf, "%s.wav", _filename);
  BinaryReader* binReader = GetBinaryReader(buf);

  if (!binReader || !binReader->IsOpen())
    return nullptr;

  auto ssd = new SoundStreamDecoder(binReader);
  if (!ssd)
    return nullptr;

  return ssd;
}

int Resource::CreateDisplayList(const char* _name)
{
  // Make sure name isn't NULL and isn't too long
  DEBUG_ASSERT(_name && strlen(_name) < 20);

  unsigned int id = glGenLists(1);
  m_displayLists.PutData(_name, id);

  return id;
}

int Resource::GetDisplayList(const char* _name)
{
  // Make sure name isn't NULL and isn't too long
  DEBUG_ASSERT(_name && strlen(_name) < 20);

  return m_displayLists.GetData(_name, -1);
}

void Resource::DeleteDisplayList(const char* _name)
{
  if (!_name)
    return;

  // Make sure name isn't too long
  DEBUG_ASSERT(strlen(_name) < 20);

  int id = m_displayLists.GetData(_name, -1);
  if (id >= 0)
  {
    glDeleteLists(id, 1);
    m_displayLists.RemoveData(_name);
  }
}

void Resource::FlushOpenGlState()
{
#if 1 // Try to catch crash on shutdown bug
  // Tell OpenGL to delete the display lists
  for (int i = 0; i < m_displayLists.Size(); ++i)
    if (m_displayLists.ValidIndex(i))
      glDeleteLists(m_displayLists[i], 1);

#endif

  // Forget all the display lists
  m_displayLists.Empty();

  // Forget all the texture handles
  m_textures.Empty();
}

void Resource::RegenerateOpenGlState()
{
  // Tell all the shapes to generate a new display list
  for (int i = 0; i < m_shapes.Size(); ++i)
  {
    if (!m_shapes.ValidIndex(i))
      continue;

    m_shapes[i]->BuildDisplayList();
  }

  // Tell the renderer (for the pixel effect texture)
  g_app->m_renderer->BuildOpenGlState();

  // Tell the location
  if (g_app->m_location)
    g_app->m_location->RegenerateOpenGlState();
}

const char* Resource::GetBaseDirectory() { return ""; }

// Finds all the filenames in the specified directory that match the specified
// filter. Directory should be like "textures" or "textures\\".
// Filter can be NULL or "" or "*.bmp" or "map_*" or "map_*.txt"
// Set _longResults to true if you want results like "textures\\blah.bmp"
// or false for "blah.bmp"
std::vector<std::string> Resource::ListResources(const char* _dir, const char* _filter, bool _longResults /* = true */)
{
  //
  // List the base data directory

  char fullDirectory[256];
  sprintf(fullDirectory, "%s", _dir);
  auto results = ListDirectory(fullDirectory, _filter, _longResults);

  return results;
}
