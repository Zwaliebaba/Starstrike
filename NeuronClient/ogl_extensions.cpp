#include "pch.h"

#include "ogl_extensions.h"

MultiTexCoord2fARB gglMultiTexCoord2fARB = nullptr;
ActiveTextureARB gglActiveTextureARB = nullptr;

glBindBufferARB gglBindBufferARB = nullptr;
glDeleteBuffersARB gglDeleteBuffersARB = nullptr;
glGenBuffersARB gglGenBuffersARB = nullptr;
glIsBufferARB gglIsBufferARB = nullptr;
glBufferDataARB gglBufferDataARB = nullptr;
glBufferSubDataARB gglBufferSubDataARB = nullptr;
glGetBufferSubDataARB gglGetBufferSubDataARB = nullptr;
glMapBufferARB gglMapBufferARB = nullptr;
glUnmapBufferARB gglUnmapBufferARB = nullptr;
glGetBufferParameterivARB gglGetBufferParameterivARB = nullptr;
glGetBufferPointervARB gglGetBufferPointervARB = nullptr;

ChoosePixelFormatARB gglChoosePixelFormatARB = nullptr;

void InitialiseOGLExtensions()
{
  gglMultiTexCoord2fARB = reinterpret_cast<MultiTexCoord2fARB>(wglGetProcAddress("glMultiTexCoord2fARB"));
  gglActiveTextureARB = reinterpret_cast<ActiveTextureARB>(wglGetProcAddress("glActiveTextureARB"));

  gglBindBufferARB = reinterpret_cast<glBindBufferARB>(wglGetProcAddress("glBindBufferARB"));
  gglDeleteBuffersARB = reinterpret_cast<glDeleteBuffersARB>(wglGetProcAddress("glDeleteBuffersARB"));
  gglGenBuffersARB = reinterpret_cast<glGenBuffersARB>(wglGetProcAddress("glGenBuffersARB"));
  gglIsBufferARB = reinterpret_cast<glIsBufferARB>(wglGetProcAddress("glIsBufferARB"));
  gglBufferDataARB = reinterpret_cast<glBufferDataARB>(wglGetProcAddress("glBufferDataARB"));
  gglBufferSubDataARB = reinterpret_cast<glBufferSubDataARB>(wglGetProcAddress("glBufferSubDataARB"));
  gglGetBufferSubDataARB = reinterpret_cast<glGetBufferSubDataARB>(wglGetProcAddress("glGetBufferSubDataARB"));
  gglMapBufferARB = reinterpret_cast<glMapBufferARB>(wglGetProcAddress("glMapBufferARB"));
  gglUnmapBufferARB = reinterpret_cast<glUnmapBufferARB>(wglGetProcAddress("glUnmapBufferARB"));
  gglGetBufferParameterivARB = reinterpret_cast<glGetBufferParameterivARB>(wglGetProcAddress("glGetBufferParameterivARB"));
  gglGetBufferPointervARB = reinterpret_cast<glGetBufferPointervARB>(wglGetProcAddress("glGetBufferPointervARB"));

  gglChoosePixelFormatARB = reinterpret_cast<ChoosePixelFormatARB>(wglGetProcAddress("wglChoosePixelFormatARB"));
}

int IsOGLExtensionSupported(const char* extension)
{
  // From http://www.opengl.org/resources/features/OGLextensions/
  const GLubyte* extensions = nullptr;
  const GLubyte* start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte*)strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;
  extensions = glGetString(GL_EXTENSIONS);
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  start = extensions;
  for (;;)
  {
    where = (GLubyte*)strstr((const char*)start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ')
    {
      if (*terminator == ' ' || *terminator == '\0')
        return 1;
    }
    start = terminator;
  }
  return 0;
}
