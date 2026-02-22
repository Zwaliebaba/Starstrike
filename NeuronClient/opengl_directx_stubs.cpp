#include "pch.h"
#include "opengl_trace.h"

const GLubyte * glGetString (GLenum name)
{
	GL_TRACE("glGetString(%s)", glEnumToString(name))
	return 0;
}

void glDeleteTextures (GLsizei n, const GLuint *textures)
{
	GL_TRACE("glDeleteTextures(%d, (const unsigned *)%p)", n, textures)
}

void glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	GL_TRACE("glFrustum(%10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g)", left, right, bottom, top, zNear, zFar)
}

void glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{
	GL_TRACE("glGetMaterialfv(%s, %s, (float *)%p)", glEnumToString(face), glEnumToString(pname), params)
}
