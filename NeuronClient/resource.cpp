#include "pch.h"
#include "binary_stream_readers.h"
#include "bitmap.h"
#include "debug_utils.h"
#include "filesys_utils.h"
#include "file_writer.h"
#include "resource.h"
#include "shape.h"
#include "text_renderer.h"
#include "text_stream_readers.h"
#include "preferences.h"
#include "sound_stream_decoder.h"
#include "texture_manager.h"
#include "app.h"
#include "location.h"
#include "renderer.h"

Resource::Resource()
  : m_nameSeed(1) {}

Resource::~Resource()
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
    reader = NEW TextFileReader(to_string(fullFilename).c_str());

  return reader;
}

BinaryReader* Resource::GetBinaryReader(std::string_view _filename)
{
  BinaryReader* reader = nullptr;
  std::string fullFilename = to_string(FileSys::GetHomeDirectory()) + std::string(_filename);

  if (DoesFileExist(fullFilename.c_str()))
    reader = NEW BinaryFileReader(fullFilename.c_str());

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
    char fullPath[512];
    sprintf(fullPath, "%s", _name);
    strlwr(fullPath);
    BinaryReader* reader = GetBinaryReader(fullPath);

    if (reader)
    {
      const char* extension = GetExtensionPart(fullPath);
      BitmapRGBA bmp(reader, extension);
      delete reader;

      if (_masked)
        bmp.ConvertPinkToTransparent();
      theTexture = bmp.ConvertToTexture(_mipMapping);
      m_textures.PutData(_name, theTexture);
    }
  }

  if (theTexture == -1)
  {
    char errorString[512];
    sprintf(errorString, "Failed to load texture %s", _name);
    DarwiniaReleaseAssert(false, errorString);
  }

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
  strlwr(fullPath);
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
    unsigned int id2 = id;
    glDeleteTextures(1, &id2);
    if (g_textureManager)
      g_textureManager->DestroyTexture(id);
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
  char fullPath[512];
  Shape* theShape = nullptr;

  if (!theShape)
  {
    hstring fullFilename = FileSys::GetHomeDirectory() + L"shapes\\" + to_hstring(_name);

    if (DoesFileExist(to_string(fullFilename).c_str()))
      theShape = NEW Shape(to_string(fullFilename).c_str(), _animating);
  }

  DarwiniaReleaseAssert(theShape, "Couldn't create shape file %s", _name);
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

int Resource::WildCmp(const char* wild, const char* string)
{
  const char* cp = nullptr;
  const char* mp = nullptr;

  while ((*string) && (*wild != '*'))
  {
    if ((*wild != *string) && (*wild != '?'))
      return 0;
    wild++;
    string++;
  }

  while (*string)
  {
    if (*wild == '*')
    {
      if (!*++wild)
        return 1;
      mp = wild;
      cp = string + 1;
    }
    else if ((*wild == *string) || (*wild == '?'))
    {
      wild++;
      string++;
    }
    else
    {
      wild = mp;
      string = cp++;
    }
  }

  while (*wild == '*') { wild++; }
  return !*wild;
}

int Resource::CreateDisplayList(const char* _name)
{
  // Make sure name isn't NULL and isn't too long
  DarwiniaDebugAssert(_name && strlen(_name) < 20);

  unsigned int id = glGenLists(1);
  m_displayLists.PutData(_name, id);

  return id;
}

int Resource::GetDisplayList(const char* _name)
{
  // Make sure name isn't NULL and isn't too long
  DarwiniaDebugAssert(_name && strlen(_name) < 20);

  return m_displayLists.GetData(_name, -1);
}

void Resource::DeleteDisplayList(const char* _name)
{
  if (!_name)
    return;

  // Make sure name isn't too long
  DarwiniaDebugAssert(strlen(_name) < 20);

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
  {
    if (m_displayLists.ValidIndex(i))
      glDeleteLists(m_displayLists[i], 1);
  }

  // Tell OpenGL to delete the textures
  for (int i = 0; i < m_textures.Size(); ++i)
  {
    if (m_textures.ValidIndex(i))
    {
      unsigned int id = m_textures[i];
      glDeleteTextures(1, &id);
      if (g_textureManager)
        g_textureManager->DestroyTexture(m_textures[i]);
    }
  }
#endif

  // Forget all the display lists
  m_displayLists.Empty();

  // Forget all the texture handles
  m_textures.Empty();

  if (g_app->m_location)
    g_app->m_location->FlushOpenGlState();
}

void Resource::RegenerateOpenGlState()
{
  // Tell the text renderers to reload their font
  g_editorFont.BuildOpenGlState();
  g_gameFont.BuildOpenGlState();

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

char* Resource::GenerateName()
{
  int digits = log10f(m_nameSeed) + 1;
  auto name = new char[digits + 1];
  itoa(m_nameSeed, name, 10);
  m_nameSeed++;

  return name;
}

// Finds all the filenames in the specified directory that match the specified
// filter. Directory should be like "textures" or "textures/".
// Filter can be NULL or "" or "*.bmp" or "map_*" or "map_*.txt"
// Set _longResults to true if you want results like "textures/blah.bmp"
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
