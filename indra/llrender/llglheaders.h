/**
 * @file llglheaders.h
 * @brief LLGL definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLGLHEADERS_H
#define LL_LLGLHEADERS_H

#if LL_MESA
//----------------------------------------------------------------------------
// MESA headers
// quotes so we get libraries/.../GL/ version
#define GL_GLEXT_PROTOTYPES 1
#include "GL/gl.h"
#include "GL/glext.h"
#include "GL/glu.h"

// The __APPLE__ kludge is to make glh_extensions.h not symbol-clash horribly
# define __APPLE__
# include "GL/glh_extensions.h"
# undef __APPLE__

#elif LL_LINUX
#define GL_GLEXT_PROTOTYPES
#define GLX_GLEXT_PROTOTYPES

#include "GL/gl.h"
#include "GL/glext.h"
#include "GL/glu.h"

// The __APPLE__ kludge is to make glh_extensions.h not symbol-clash horribly
# define __APPLE__
# include "GL/glh_extensions.h"
# undef __APPLE__

# include "GL/glx.h"
# include "GL/glxext.h"

#elif LL_WINDOWS
//----------------------------------------------------------------------------
// LL_WINDOWS

// windows gl headers depend on things like APIENTRY, so include windows.
#include "llwin32headers.h"

//----------------------------------------------------------------------------
#include <GL/gl.h>
#include <GL/glu.h>

// quotes so we get libraries/.../GL/ version
#include "GL/glext.h"
#include "GL/glh_extensions.h"

#define LL_THREAD_LOCAL_GL  // thread_local  // <-- uncomment to trigger a crash whenever calling a GL function from a non-GL thread

// WGL_AMD_gpu_association
extern LL_THREAD_LOCAL_GL PFNWGLGETGPUIDSAMDPROC                          wglGetGPUIDsAMD;
extern LL_THREAD_LOCAL_GL PFNWGLGETGPUINFOAMDPROC                         wglGetGPUInfoAMD;
extern LL_THREAD_LOCAL_GL PFNWGLGETCONTEXTGPUIDAMDPROC                    wglGetContextGPUIDAMD;
extern LL_THREAD_LOCAL_GL PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC            wglCreateAssociatedContextAMD;
extern LL_THREAD_LOCAL_GL PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC     wglCreateAssociatedContextAttribsAMD;
extern LL_THREAD_LOCAL_GL PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC            wglDeleteAssociatedContextAMD;
extern LL_THREAD_LOCAL_GL PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC       wglMakeAssociatedContextCurrentAMD;
extern LL_THREAD_LOCAL_GL PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC        wglGetCurrentAssociatedContextAMD;
extern LL_THREAD_LOCAL_GL PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC             wglBlitContextFramebufferAMD;

// WGL_EXT_swap_control
extern LL_THREAD_LOCAL_GL PFNWGLSWAPINTERVALEXTPROC    wglSwapIntervalEXT;
extern LL_THREAD_LOCAL_GL PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

// WGL_ARB_create_context
extern LL_THREAD_LOCAL_GL PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

// GL_VERSION_1_3
extern LL_THREAD_LOCAL_GL PFNGLACTIVETEXTUREPROC               glActiveTexture;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLECOVERAGEPROC              glSampleCoverage;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXIMAGE3DPROC        glCompressedTexImage3D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXIMAGE2DPROC        glCompressedTexImage2D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXIMAGE1DPROC        glCompressedTexImage1D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC     glCompressedTexSubImage3D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC     glCompressedTexSubImage2D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC     glCompressedTexSubImage1D;
extern LL_THREAD_LOCAL_GL PFNGLGETCOMPRESSEDTEXIMAGEPROC       glGetCompressedTexImage;
extern LL_THREAD_LOCAL_GL PFNGLCLIENTACTIVETEXTUREPROC         glClientActiveTexture;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1DPROC             glMultiTexCoord1d;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1DVPROC            glMultiTexCoord1dv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1FPROC             glMultiTexCoord1f;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1FVPROC            glMultiTexCoord1fv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1IPROC             glMultiTexCoord1i;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1IVPROC            glMultiTexCoord1iv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1SPROC             glMultiTexCoord1s;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD1SVPROC            glMultiTexCoord1sv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2DPROC             glMultiTexCoord2d;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2DVPROC            glMultiTexCoord2dv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2FPROC             glMultiTexCoord2f;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2FVPROC            glMultiTexCoord2fv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2IPROC             glMultiTexCoord2i;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2IVPROC            glMultiTexCoord2iv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2SPROC             glMultiTexCoord2s;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD2SVPROC            glMultiTexCoord2sv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3DPROC             glMultiTexCoord3d;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3DVPROC            glMultiTexCoord3dv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3FPROC             glMultiTexCoord3f;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3FVPROC            glMultiTexCoord3fv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3IPROC             glMultiTexCoord3i;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3IVPROC            glMultiTexCoord3iv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3SPROC             glMultiTexCoord3s;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD3SVPROC            glMultiTexCoord3sv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4DPROC             glMultiTexCoord4d;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4DVPROC            glMultiTexCoord4dv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4FPROC             glMultiTexCoord4f;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4FVPROC            glMultiTexCoord4fv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4IPROC             glMultiTexCoord4i;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4IVPROC            glMultiTexCoord4iv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4SPROC             glMultiTexCoord4s;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORD4SVPROC            glMultiTexCoord4sv;
extern LL_THREAD_LOCAL_GL PFNGLLOADTRANSPOSEMATRIXFPROC        glLoadTransposeMatrixf;
extern LL_THREAD_LOCAL_GL PFNGLLOADTRANSPOSEMATRIXDPROC        glLoadTransposeMatrixd;
extern LL_THREAD_LOCAL_GL PFNGLMULTTRANSPOSEMATRIXFPROC        glMultTransposeMatrixf;
extern LL_THREAD_LOCAL_GL PFNGLMULTTRANSPOSEMATRIXDPROC        glMultTransposeMatrixd;

// GL_VERSION_1_4
extern LL_THREAD_LOCAL_GL PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;
extern LL_THREAD_LOCAL_GL PFNGLMULTIDRAWARRAYSPROC         glMultiDrawArrays;
extern LL_THREAD_LOCAL_GL PFNGLMULTIDRAWELEMENTSPROC       glMultiDrawElements;
extern LL_THREAD_LOCAL_GL PFNGLPOINTPARAMETERFPROC         glPointParameterf;
extern LL_THREAD_LOCAL_GL PFNGLPOINTPARAMETERFVPROC        glPointParameterfv;
extern LL_THREAD_LOCAL_GL PFNGLPOINTPARAMETERIPROC         glPointParameteri;
extern LL_THREAD_LOCAL_GL PFNGLPOINTPARAMETERIVPROC        glPointParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLFOGCOORDFPROC               glFogCoordf;
extern LL_THREAD_LOCAL_GL PFNGLFOGCOORDFVPROC              glFogCoordfv;
extern LL_THREAD_LOCAL_GL PFNGLFOGCOORDDPROC               glFogCoordd;
extern LL_THREAD_LOCAL_GL PFNGLFOGCOORDDVPROC              glFogCoorddv;
extern LL_THREAD_LOCAL_GL PFNGLFOGCOORDPOINTERPROC         glFogCoordPointer;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3BPROC        glSecondaryColor3b;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3BVPROC       glSecondaryColor3bv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3DPROC        glSecondaryColor3d;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3DVPROC       glSecondaryColor3dv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3FPROC        glSecondaryColor3f;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3FVPROC       glSecondaryColor3fv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3IPROC        glSecondaryColor3i;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3IVPROC       glSecondaryColor3iv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3SPROC        glSecondaryColor3s;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3SVPROC       glSecondaryColor3sv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3UBPROC       glSecondaryColor3ub;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3UBVPROC      glSecondaryColor3ubv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3UIPROC       glSecondaryColor3ui;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3UIVPROC      glSecondaryColor3uiv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3USPROC       glSecondaryColor3us;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLOR3USVPROC      glSecondaryColor3usv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLORPOINTERPROC   glSecondaryColorPointer;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2DPROC             glWindowPos2d;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2DVPROC            glWindowPos2dv;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2FPROC             glWindowPos2f;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2FVPROC            glWindowPos2fv;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2IPROC             glWindowPos2i;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2IVPROC            glWindowPos2iv;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2SPROC             glWindowPos2s;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS2SVPROC            glWindowPos2sv;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3DPROC             glWindowPos3d;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3DVPROC            glWindowPos3dv;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3FPROC             glWindowPos3f;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3FVPROC            glWindowPos3fv;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3IPROC             glWindowPos3i;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3IVPROC            glWindowPos3iv;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3SPROC             glWindowPos3s;
extern LL_THREAD_LOCAL_GL PFNGLWINDOWPOS3SVPROC            glWindowPos3sv;

// GL_VERSION_1_5
extern LL_THREAD_LOCAL_GL PFNGLGENQUERIESPROC              glGenQueries;
extern LL_THREAD_LOCAL_GL PFNGLDELETEQUERIESPROC           glDeleteQueries;
extern LL_THREAD_LOCAL_GL PFNGLISQUERYPROC                 glIsQuery;
extern LL_THREAD_LOCAL_GL PFNGLBEGINQUERYPROC              glBeginQuery;
extern LL_THREAD_LOCAL_GL PFNGLENDQUERYPROC                glEndQuery;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYIVPROC              glGetQueryiv;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYOBJECTIVPROC        glGetQueryObjectiv;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYOBJECTUIVPROC       glGetQueryObjectuiv;
extern LL_THREAD_LOCAL_GL PFNGLBINDBUFFERPROC              glBindBuffer;
extern LL_THREAD_LOCAL_GL PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
extern LL_THREAD_LOCAL_GL PFNGLGENBUFFERSPROC              glGenBuffers;
extern LL_THREAD_LOCAL_GL PFNGLISBUFFERPROC                glIsBuffer;
extern LL_THREAD_LOCAL_GL PFNGLBUFFERDATAPROC              glBufferData;
extern LL_THREAD_LOCAL_GL PFNGLBUFFERSUBDATAPROC           glBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLGETBUFFERSUBDATAPROC        glGetBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLMAPBUFFERPROC               glMapBuffer;
extern LL_THREAD_LOCAL_GL PFNGLUNMAPBUFFERPROC             glUnmapBuffer;
extern LL_THREAD_LOCAL_GL PFNGLGETBUFFERPARAMETERIVPROC    glGetBufferParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGETBUFFERPOINTERVPROC       glGetBufferPointerv;

// GL_VERSION_2_0
extern LL_THREAD_LOCAL_GL PFNGLBLENDEQUATIONSEPARATEPROC           glBlendEquationSeparate;
extern LL_THREAD_LOCAL_GL PFNGLDRAWBUFFERSPROC                     glDrawBuffers;
extern LL_THREAD_LOCAL_GL PFNGLSTENCILOPSEPARATEPROC               glStencilOpSeparate;
extern LL_THREAD_LOCAL_GL PFNGLSTENCILFUNCSEPARATEPROC             glStencilFuncSeparate;
extern LL_THREAD_LOCAL_GL PFNGLSTENCILMASKSEPARATEPROC             glStencilMaskSeparate;
extern LL_THREAD_LOCAL_GL PFNGLATTACHSHADERPROC                    glAttachShader;
extern LL_THREAD_LOCAL_GL PFNGLBINDATTRIBLOCATIONPROC              glBindAttribLocation;
extern LL_THREAD_LOCAL_GL PFNGLCOMPILESHADERPROC                   glCompileShader;
extern LL_THREAD_LOCAL_GL PFNGLCREATEPROGRAMPROC                   glCreateProgram;
extern LL_THREAD_LOCAL_GL PFNGLCREATESHADERPROC                    glCreateShader;
extern LL_THREAD_LOCAL_GL PFNGLDELETEPROGRAMPROC                   glDeleteProgram;
extern LL_THREAD_LOCAL_GL PFNGLDELETESHADERPROC                    glDeleteShader;
extern LL_THREAD_LOCAL_GL PFNGLDETACHSHADERPROC                    glDetachShader;
extern LL_THREAD_LOCAL_GL PFNGLDISABLEVERTEXATTRIBARRAYPROC        glDisableVertexAttribArray;
extern LL_THREAD_LOCAL_GL PFNGLENABLEVERTEXATTRIBARRAYPROC         glEnableVertexAttribArray;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVEATTRIBPROC                 glGetActiveAttrib;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVEUNIFORMPROC                glGetActiveUniform;
extern LL_THREAD_LOCAL_GL PFNGLGETATTACHEDSHADERSPROC              glGetAttachedShaders;
extern LL_THREAD_LOCAL_GL PFNGLGETATTRIBLOCATIONPROC               glGetAttribLocation;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMIVPROC                    glGetProgramiv;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMINFOLOGPROC               glGetProgramInfoLog;
extern LL_THREAD_LOCAL_GL PFNGLGETSHADERIVPROC                     glGetShaderiv;
extern LL_THREAD_LOCAL_GL PFNGLGETSHADERINFOLOGPROC                glGetShaderInfoLog;
extern LL_THREAD_LOCAL_GL PFNGLGETSHADERSOURCEPROC                 glGetShaderSource;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMLOCATIONPROC              glGetUniformLocation;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMFVPROC                    glGetUniformfv;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMIVPROC                    glGetUniformiv;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXATTRIBDVPROC               glGetVertexAttribdv;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXATTRIBFVPROC               glGetVertexAttribfv;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXATTRIBIVPROC               glGetVertexAttribiv;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXATTRIBPOINTERVPROC         glGetVertexAttribPointerv;
extern LL_THREAD_LOCAL_GL PFNGLISPROGRAMPROC                       glIsProgram;
extern LL_THREAD_LOCAL_GL PFNGLISSHADERPROC                        glIsShader;
extern LL_THREAD_LOCAL_GL PFNGLLINKPROGRAMPROC                     glLinkProgram;
extern LL_THREAD_LOCAL_GL PFNGLSHADERSOURCEPROC                    glShaderSource;
extern LL_THREAD_LOCAL_GL PFNGLUSEPROGRAMPROC                      glUseProgram;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1FPROC                       glUniform1f;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2FPROC                       glUniform2f;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3FPROC                       glUniform3f;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4FPROC                       glUniform4f;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1IPROC                       glUniform1i;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2IPROC                       glUniform2i;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3IPROC                       glUniform3i;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4IPROC                       glUniform4i;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1FVPROC                      glUniform1fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2FVPROC                      glUniform2fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3FVPROC                      glUniform3fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4FVPROC                      glUniform4fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1IVPROC                      glUniform1iv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2IVPROC                      glUniform2iv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3IVPROC                      glUniform3iv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4IVPROC                      glUniform4iv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX2FVPROC                glUniformMatrix2fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX3FVPROC                glUniformMatrix3fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX4FVPROC                glUniformMatrix4fv;
extern LL_THREAD_LOCAL_GL PFNGLVALIDATEPROGRAMPROC                 glValidateProgram;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB1DPROC                  glVertexAttrib1d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB1DVPROC                 glVertexAttrib1dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB1FPROC                  glVertexAttrib1f;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB1FVPROC                 glVertexAttrib1fv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB1SPROC                  glVertexAttrib1s;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB1SVPROC                 glVertexAttrib1sv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB2DPROC                  glVertexAttrib2d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB2DVPROC                 glVertexAttrib2dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB2FPROC                  glVertexAttrib2f;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB2FVPROC                 glVertexAttrib2fv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB2SPROC                  glVertexAttrib2s;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB2SVPROC                 glVertexAttrib2sv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB3DPROC                  glVertexAttrib3d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB3DVPROC                 glVertexAttrib3dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB3FPROC                  glVertexAttrib3f;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB3FVPROC                 glVertexAttrib3fv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB3SPROC                  glVertexAttrib3s;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB3SVPROC                 glVertexAttrib3sv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4NBVPROC                glVertexAttrib4Nbv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4NIVPROC                glVertexAttrib4Niv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4NSVPROC                glVertexAttrib4Nsv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4NUBPROC                glVertexAttrib4Nub;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4NUBVPROC               glVertexAttrib4Nubv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4NUIVPROC               glVertexAttrib4Nuiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4NUSVPROC               glVertexAttrib4Nusv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4BVPROC                 glVertexAttrib4bv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4DPROC                  glVertexAttrib4d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4DVPROC                 glVertexAttrib4dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4FPROC                  glVertexAttrib4f;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4FVPROC                 glVertexAttrib4fv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4IVPROC                 glVertexAttrib4iv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4SPROC                  glVertexAttrib4s;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4SVPROC                 glVertexAttrib4sv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4UBVPROC                glVertexAttrib4ubv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4UIVPROC                glVertexAttrib4uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIB4USVPROC                glVertexAttrib4usv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBPOINTERPROC             glVertexAttribPointer;

// GL_VERSION_2_1
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv;

// GL_VERSION_3_0
extern LL_THREAD_LOCAL_GL PFNGLCOLORMASKIPROC                              glColorMaski;
extern LL_THREAD_LOCAL_GL PFNGLGETBOOLEANI_VPROC                           glGetBooleani_v;
extern LL_THREAD_LOCAL_GL PFNGLGETINTEGERI_VPROC                           glGetIntegeri_v;
extern LL_THREAD_LOCAL_GL PFNGLENABLEIPROC                                 glEnablei;
extern LL_THREAD_LOCAL_GL PFNGLDISABLEIPROC                                glDisablei;
extern LL_THREAD_LOCAL_GL PFNGLISENABLEDIPROC                              glIsEnabledi;
extern LL_THREAD_LOCAL_GL PFNGLBEGINTRANSFORMFEEDBACKPROC                  glBeginTransformFeedback;
extern LL_THREAD_LOCAL_GL PFNGLENDTRANSFORMFEEDBACKPROC                    glEndTransformFeedback;
extern LL_THREAD_LOCAL_GL PFNGLBINDBUFFERRANGEPROC                         glBindBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLBINDBUFFERBASEPROC                          glBindBufferBase;
extern LL_THREAD_LOCAL_GL PFNGLTRANSFORMFEEDBACKVARYINGSPROC               glTransformFeedbackVaryings;
extern LL_THREAD_LOCAL_GL PFNGLGETTRANSFORMFEEDBACKVARYINGPROC             glGetTransformFeedbackVarying;
extern LL_THREAD_LOCAL_GL PFNGLCLAMPCOLORPROC                              glClampColor;
extern LL_THREAD_LOCAL_GL PFNGLBEGINCONDITIONALRENDERPROC                  glBeginConditionalRender;
extern LL_THREAD_LOCAL_GL PFNGLENDCONDITIONALRENDERPROC                    glEndConditionalRender;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBIPOINTERPROC                    glVertexAttribIPointer;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXATTRIBIIVPROC                      glGetVertexAttribIiv;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXATTRIBIUIVPROC                     glGetVertexAttribIuiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI1IPROC                         glVertexAttribI1i;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI2IPROC                         glVertexAttribI2i;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI3IPROC                         glVertexAttribI3i;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4IPROC                         glVertexAttribI4i;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI1UIPROC                        glVertexAttribI1ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI2UIPROC                        glVertexAttribI2ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI3UIPROC                        glVertexAttribI3ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4UIPROC                        glVertexAttribI4ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI1IVPROC                        glVertexAttribI1iv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI2IVPROC                        glVertexAttribI2iv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI3IVPROC                        glVertexAttribI3iv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4IVPROC                        glVertexAttribI4iv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI1UIVPROC                       glVertexAttribI1uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI2UIVPROC                       glVertexAttribI2uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI3UIVPROC                       glVertexAttribI3uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4UIVPROC                       glVertexAttribI4uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4BVPROC                        glVertexAttribI4bv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4SVPROC                        glVertexAttribI4sv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4UBVPROC                       glVertexAttribI4ubv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBI4USVPROC                       glVertexAttribI4usv;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMUIVPROC                           glGetUniformuiv;
extern LL_THREAD_LOCAL_GL PFNGLBINDFRAGDATALOCATIONPROC                    glBindFragDataLocation;
extern LL_THREAD_LOCAL_GL PFNGLGETFRAGDATALOCATIONPROC                     glGetFragDataLocation;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1UIPROC                              glUniform1ui;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2UIPROC                              glUniform2ui;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3UIPROC                              glUniform3ui;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4UIPROC                              glUniform4ui;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1UIVPROC                             glUniform1uiv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2UIVPROC                             glUniform2uiv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3UIVPROC                             glUniform3uiv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4UIVPROC                             glUniform4uiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXPARAMETERIIVPROC                         glTexParameterIiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXPARAMETERIUIVPROC                        glTexParameterIuiv;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXPARAMETERIIVPROC                      glGetTexParameterIiv;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXPARAMETERIUIVPROC                     glGetTexParameterIuiv;
extern LL_THREAD_LOCAL_GL PFNGLCLEARBUFFERIVPROC                           glClearBufferiv;
extern LL_THREAD_LOCAL_GL PFNGLCLEARBUFFERUIVPROC                          glClearBufferuiv;
extern LL_THREAD_LOCAL_GL PFNGLCLEARBUFFERFVPROC                           glClearBufferfv;
extern LL_THREAD_LOCAL_GL PFNGLCLEARBUFFERFIPROC                           glClearBufferfi;
extern LL_THREAD_LOCAL_GL PFNGLGETSTRINGIPROC                              glGetStringi;
extern LL_THREAD_LOCAL_GL PFNGLISRENDERBUFFERPROC                          glIsRenderbuffer;
extern LL_THREAD_LOCAL_GL PFNGLBINDRENDERBUFFERPROC                        glBindRenderbuffer;
extern LL_THREAD_LOCAL_GL PFNGLDELETERENDERBUFFERSPROC                     glDeleteRenderbuffers;
extern LL_THREAD_LOCAL_GL PFNGLGENRENDERBUFFERSPROC                        glGenRenderbuffers;
extern LL_THREAD_LOCAL_GL PFNGLRENDERBUFFERSTORAGEPROC                     glRenderbufferStorage;
extern LL_THREAD_LOCAL_GL PFNGLGETRENDERBUFFERPARAMETERIVPROC              glGetRenderbufferParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLISFRAMEBUFFERPROC                           glIsFramebuffer;
extern LL_THREAD_LOCAL_GL PFNGLBINDFRAMEBUFFERPROC                         glBindFramebuffer;
extern LL_THREAD_LOCAL_GL PFNGLDELETEFRAMEBUFFERSPROC                      glDeleteFramebuffers;
extern LL_THREAD_LOCAL_GL PFNGLGENFRAMEBUFFERSPROC                         glGenFramebuffers;
extern LL_THREAD_LOCAL_GL PFNGLCHECKFRAMEBUFFERSTATUSPROC                  glCheckFramebufferStatus;
extern LL_THREAD_LOCAL_GL PFNGLFRAMEBUFFERTEXTURE1DPROC                    glFramebufferTexture1D;
extern LL_THREAD_LOCAL_GL PFNGLFRAMEBUFFERTEXTURE2DPROC                    glFramebufferTexture2D;
extern LL_THREAD_LOCAL_GL PFNGLFRAMEBUFFERTEXTURE3DPROC                    glFramebufferTexture3D;
extern LL_THREAD_LOCAL_GL PFNGLFRAMEBUFFERRENDERBUFFERPROC                 glFramebufferRenderbuffer;
extern LL_THREAD_LOCAL_GL PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC     glGetFramebufferAttachmentParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGENERATEMIPMAPPROC                          glGenerateMipmap;
extern LL_THREAD_LOCAL_GL PFNGLBLITFRAMEBUFFERPROC                         glBlitFramebuffer;
extern LL_THREAD_LOCAL_GL PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC          glRenderbufferStorageMultisample;
extern LL_THREAD_LOCAL_GL PFNGLFRAMEBUFFERTEXTURELAYERPROC                 glFramebufferTextureLayer;
extern LL_THREAD_LOCAL_GL PFNGLMAPBUFFERRANGEPROC                          glMapBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLFLUSHMAPPEDBUFFERRANGEPROC                  glFlushMappedBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLBINDVERTEXARRAYPROC                         glBindVertexArray;
extern LL_THREAD_LOCAL_GL PFNGLDELETEVERTEXARRAYSPROC                      glDeleteVertexArrays;
extern LL_THREAD_LOCAL_GL PFNGLGENVERTEXARRAYSPROC                         glGenVertexArrays;
extern LL_THREAD_LOCAL_GL PFNGLISVERTEXARRAYPROC                           glIsVertexArray;

// GL_VERSION_3_1
extern LL_THREAD_LOCAL_GL PFNGLDRAWARRAYSINSTANCEDPROC         glDrawArraysInstanced;
extern LL_THREAD_LOCAL_GL PFNGLDRAWELEMENTSINSTANCEDPROC       glDrawElementsInstanced;
extern LL_THREAD_LOCAL_GL PFNGLTEXBUFFERPROC                   glTexBuffer;
extern LL_THREAD_LOCAL_GL PFNGLPRIMITIVERESTARTINDEXPROC       glPrimitiveRestartIndex;
extern LL_THREAD_LOCAL_GL PFNGLCOPYBUFFERSUBDATAPROC           glCopyBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMINDICESPROC           glGetUniformIndices;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVEUNIFORMSIVPROC         glGetActiveUniformsiv;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVEUNIFORMNAMEPROC        glGetActiveUniformName;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMBLOCKINDEXPROC        glGetUniformBlockIndex;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVEUNIFORMBLOCKIVPROC     glGetActiveUniformBlockiv;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC   glGetActiveUniformBlockName;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMBLOCKBINDINGPROC         glUniformBlockBinding;

// GL_VERSION_3_2
extern LL_THREAD_LOCAL_GL PFNGLDRAWELEMENTSBASEVERTEXPROC          glDrawElementsBaseVertex;
extern LL_THREAD_LOCAL_GL PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC     glDrawRangeElementsBaseVertex;
extern LL_THREAD_LOCAL_GL PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glDrawElementsInstancedBaseVertex;
extern LL_THREAD_LOCAL_GL PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC     glMultiDrawElementsBaseVertex;
extern LL_THREAD_LOCAL_GL PFNGLPROVOKINGVERTEXPROC                 glProvokingVertex;
extern LL_THREAD_LOCAL_GL PFNGLFENCESYNCPROC                       glFenceSync;
extern LL_THREAD_LOCAL_GL PFNGLISSYNCPROC                          glIsSync;
extern LL_THREAD_LOCAL_GL PFNGLDELETESYNCPROC                      glDeleteSync;
extern LL_THREAD_LOCAL_GL PFNGLCLIENTWAITSYNCPROC                  glClientWaitSync;
extern LL_THREAD_LOCAL_GL PFNGLWAITSYNCPROC                        glWaitSync;
extern LL_THREAD_LOCAL_GL PFNGLGETINTEGER64VPROC                   glGetInteger64v;
extern LL_THREAD_LOCAL_GL PFNGLGETSYNCIVPROC                       glGetSynciv;
extern LL_THREAD_LOCAL_GL PFNGLGETINTEGER64I_VPROC                 glGetInteger64i_v;
extern LL_THREAD_LOCAL_GL PFNGLGETBUFFERPARAMETERI64VPROC          glGetBufferParameteri64v;
extern LL_THREAD_LOCAL_GL PFNGLFRAMEBUFFERTEXTUREPROC              glFramebufferTexture;
extern LL_THREAD_LOCAL_GL PFNGLTEXIMAGE2DMULTISAMPLEPROC           glTexImage2DMultisample;
extern LL_THREAD_LOCAL_GL PFNGLTEXIMAGE3DMULTISAMPLEPROC           glTexImage3DMultisample;
extern LL_THREAD_LOCAL_GL PFNGLGETMULTISAMPLEFVPROC                glGetMultisamplefv;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLEMASKIPROC                     glSampleMaski;

// GL_VERSION_3_3
extern LL_THREAD_LOCAL_GL PFNGLBINDFRAGDATALOCATIONINDEXEDPROC  glBindFragDataLocationIndexed;
extern LL_THREAD_LOCAL_GL PFNGLGETFRAGDATAINDEXPROC             glGetFragDataIndex;
extern LL_THREAD_LOCAL_GL PFNGLGENSAMPLERSPROC                  glGenSamplers;
extern LL_THREAD_LOCAL_GL PFNGLDELETESAMPLERSPROC               glDeleteSamplers;
extern LL_THREAD_LOCAL_GL PFNGLISSAMPLERPROC                    glIsSampler;
extern LL_THREAD_LOCAL_GL PFNGLBINDSAMPLERPROC                  glBindSampler;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLERPARAMETERIPROC            glSamplerParameteri;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLERPARAMETERIVPROC           glSamplerParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLERPARAMETERFPROC            glSamplerParameterf;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLERPARAMETERFVPROC           glSamplerParameterfv;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLERPARAMETERIIVPROC          glSamplerParameterIiv;
extern LL_THREAD_LOCAL_GL PFNGLSAMPLERPARAMETERIUIVPROC         glSamplerParameterIuiv;
extern LL_THREAD_LOCAL_GL PFNGLGETSAMPLERPARAMETERIVPROC        glGetSamplerParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGETSAMPLERPARAMETERIIVPROC       glGetSamplerParameterIiv;
extern LL_THREAD_LOCAL_GL PFNGLGETSAMPLERPARAMETERFVPROC        glGetSamplerParameterfv;
extern LL_THREAD_LOCAL_GL PFNGLGETSAMPLERPARAMETERIUIVPROC      glGetSamplerParameterIuiv;
extern LL_THREAD_LOCAL_GL PFNGLQUERYCOUNTERPROC                 glQueryCounter;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYOBJECTI64VPROC           glGetQueryObjecti64v;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYOBJECTUI64VPROC          glGetQueryObjectui64v;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBDIVISORPROC          glVertexAttribDivisor;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP1UIPROC             glVertexAttribP1ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP1UIVPROC            glVertexAttribP1uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP2UIPROC             glVertexAttribP2ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP2UIVPROC            glVertexAttribP2uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP3UIPROC             glVertexAttribP3ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP3UIVPROC            glVertexAttribP3uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP4UIPROC             glVertexAttribP4ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBP4UIVPROC            glVertexAttribP4uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXP2UIPROC                   glVertexP2ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXP2UIVPROC                  glVertexP2uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXP3UIPROC                   glVertexP3ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXP3UIVPROC                  glVertexP3uiv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXP4UIPROC                   glVertexP4ui;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXP4UIVPROC                  glVertexP4uiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP1UIPROC                 glTexCoordP1ui;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP1UIVPROC                glTexCoordP1uiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP2UIPROC                 glTexCoordP2ui;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP2UIVPROC                glTexCoordP2uiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP3UIPROC                 glTexCoordP3ui;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP3UIVPROC                glTexCoordP3uiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP4UIPROC                 glTexCoordP4ui;
extern LL_THREAD_LOCAL_GL PFNGLTEXCOORDP4UIVPROC                glTexCoordP4uiv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP1UIPROC            glMultiTexCoordP1ui;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP1UIVPROC           glMultiTexCoordP1uiv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP2UIPROC            glMultiTexCoordP2ui;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP2UIVPROC           glMultiTexCoordP2uiv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP3UIPROC            glMultiTexCoordP3ui;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP3UIVPROC           glMultiTexCoordP3uiv;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP4UIPROC            glMultiTexCoordP4ui;
extern LL_THREAD_LOCAL_GL PFNGLMULTITEXCOORDP4UIVPROC           glMultiTexCoordP4uiv;
extern LL_THREAD_LOCAL_GL PFNGLNORMALP3UIPROC                   glNormalP3ui;
extern LL_THREAD_LOCAL_GL PFNGLNORMALP3UIVPROC                  glNormalP3uiv;
extern LL_THREAD_LOCAL_GL PFNGLCOLORP3UIPROC                    glColorP3ui;
extern LL_THREAD_LOCAL_GL PFNGLCOLORP3UIVPROC                   glColorP3uiv;
extern LL_THREAD_LOCAL_GL PFNGLCOLORP4UIPROC                    glColorP4ui;
extern LL_THREAD_LOCAL_GL PFNGLCOLORP4UIVPROC                   glColorP4uiv;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLORP3UIPROC           glSecondaryColorP3ui;
extern LL_THREAD_LOCAL_GL PFNGLSECONDARYCOLORP3UIVPROC          glSecondaryColorP3uiv;

// GL_VERSION_4_0
extern LL_THREAD_LOCAL_GL PFNGLMINSAMPLESHADINGPROC                glMinSampleShading;
extern LL_THREAD_LOCAL_GL PFNGLBLENDEQUATIONIPROC                  glBlendEquationi;
extern LL_THREAD_LOCAL_GL PFNGLBLENDEQUATIONSEPARATEIPROC          glBlendEquationSeparatei;
extern LL_THREAD_LOCAL_GL PFNGLBLENDFUNCIPROC                      glBlendFunci;
extern LL_THREAD_LOCAL_GL PFNGLBLENDFUNCSEPARATEIPROC              glBlendFuncSeparatei;
extern LL_THREAD_LOCAL_GL PFNGLDRAWARRAYSINDIRECTPROC              glDrawArraysIndirect;
extern LL_THREAD_LOCAL_GL PFNGLDRAWELEMENTSINDIRECTPROC            glDrawElementsIndirect;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1DPROC                       glUniform1d;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2DPROC                       glUniform2d;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3DPROC                       glUniform3d;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4DPROC                       glUniform4d;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM1DVPROC                      glUniform1dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM2DVPROC                      glUniform2dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM3DVPROC                      glUniform3dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORM4DVPROC                      glUniform4dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX2DVPROC                glUniformMatrix2dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX3DVPROC                glUniformMatrix3dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX4DVPROC                glUniformMatrix4dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX2X3DVPROC              glUniformMatrix2x3dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX2X4DVPROC              glUniformMatrix2x4dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX3X2DVPROC              glUniformMatrix3x2dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX3X4DVPROC              glUniformMatrix3x4dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX4X2DVPROC              glUniformMatrix4x2dv;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMMATRIX4X3DVPROC              glUniformMatrix4x3dv;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMDVPROC                    glGetUniformdv;
extern LL_THREAD_LOCAL_GL PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC    glGetSubroutineUniformLocation;
extern LL_THREAD_LOCAL_GL PFNGLGETSUBROUTINEINDEXPROC              glGetSubroutineIndex;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC    glGetActiveSubroutineUniformiv;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC  glGetActiveSubroutineUniformName;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVESUBROUTINENAMEPROC         glGetActiveSubroutineName;
extern LL_THREAD_LOCAL_GL PFNGLUNIFORMSUBROUTINESUIVPROC           glUniformSubroutinesuiv;
extern LL_THREAD_LOCAL_GL PFNGLGETUNIFORMSUBROUTINEUIVPROC         glGetUniformSubroutineuiv;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMSTAGEIVPROC               glGetProgramStageiv;
extern LL_THREAD_LOCAL_GL PFNGLPATCHPARAMETERIPROC                 glPatchParameteri;
extern LL_THREAD_LOCAL_GL PFNGLPATCHPARAMETERFVPROC                glPatchParameterfv;
extern LL_THREAD_LOCAL_GL PFNGLBINDTRANSFORMFEEDBACKPROC           glBindTransformFeedback;
extern LL_THREAD_LOCAL_GL PFNGLDELETETRANSFORMFEEDBACKSPROC        glDeleteTransformFeedbacks;
extern LL_THREAD_LOCAL_GL PFNGLGENTRANSFORMFEEDBACKSPROC           glGenTransformFeedbacks;
extern LL_THREAD_LOCAL_GL PFNGLISTRANSFORMFEEDBACKPROC             glIsTransformFeedback;
extern LL_THREAD_LOCAL_GL PFNGLPAUSETRANSFORMFEEDBACKPROC          glPauseTransformFeedback;
extern LL_THREAD_LOCAL_GL PFNGLRESUMETRANSFORMFEEDBACKPROC         glResumeTransformFeedback;
extern LL_THREAD_LOCAL_GL PFNGLDRAWTRANSFORMFEEDBACKPROC           glDrawTransformFeedback;
extern LL_THREAD_LOCAL_GL PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC     glDrawTransformFeedbackStream;
extern LL_THREAD_LOCAL_GL PFNGLBEGINQUERYINDEXEDPROC               glBeginQueryIndexed;
extern LL_THREAD_LOCAL_GL PFNGLENDQUERYINDEXEDPROC                 glEndQueryIndexed;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYINDEXEDIVPROC               glGetQueryIndexediv;

 // GL_VERSION_4_1
extern LL_THREAD_LOCAL_GL PFNGLRELEASESHADERCOMPILERPROC           glReleaseShaderCompiler;
extern LL_THREAD_LOCAL_GL PFNGLSHADERBINARYPROC                    glShaderBinary;
extern LL_THREAD_LOCAL_GL PFNGLGETSHADERPRECISIONFORMATPROC        glGetShaderPrecisionFormat;
extern LL_THREAD_LOCAL_GL PFNGLDEPTHRANGEFPROC                     glDepthRangef;
extern LL_THREAD_LOCAL_GL PFNGLCLEARDEPTHFPROC                     glClearDepthf;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMBINARYPROC                glGetProgramBinary;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMBINARYPROC                   glProgramBinary;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMPARAMETERIPROC               glProgramParameteri;
extern LL_THREAD_LOCAL_GL PFNGLUSEPROGRAMSTAGESPROC                glUseProgramStages;
extern LL_THREAD_LOCAL_GL PFNGLACTIVESHADERPROGRAMPROC             glActiveShaderProgram;
extern LL_THREAD_LOCAL_GL PFNGLCREATESHADERPROGRAMVPROC            glCreateShaderProgramv;
extern LL_THREAD_LOCAL_GL PFNGLBINDPROGRAMPIPELINEPROC             glBindProgramPipeline;
extern LL_THREAD_LOCAL_GL PFNGLDELETEPROGRAMPIPELINESPROC          glDeleteProgramPipelines;
extern LL_THREAD_LOCAL_GL PFNGLGENPROGRAMPIPELINESPROC             glGenProgramPipelines;
extern LL_THREAD_LOCAL_GL PFNGLISPROGRAMPIPELINEPROC               glIsProgramPipeline;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMPIPELINEIVPROC            glGetProgramPipelineiv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1IPROC                glProgramUniform1i;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1IVPROC               glProgramUniform1iv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1FPROC                glProgramUniform1f;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1FVPROC               glProgramUniform1fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1DPROC                glProgramUniform1d;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1DVPROC               glProgramUniform1dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1UIPROC               glProgramUniform1ui;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM1UIVPROC              glProgramUniform1uiv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2IPROC                glProgramUniform2i;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2IVPROC               glProgramUniform2iv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2FPROC                glProgramUniform2f;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2FVPROC               glProgramUniform2fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2DPROC                glProgramUniform2d;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2DVPROC               glProgramUniform2dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2UIPROC               glProgramUniform2ui;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM2UIVPROC              glProgramUniform2uiv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3IPROC                glProgramUniform3i;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3IVPROC               glProgramUniform3iv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3FPROC                glProgramUniform3f;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3FVPROC               glProgramUniform3fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3DPROC                glProgramUniform3d;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3DVPROC               glProgramUniform3dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3UIPROC               glProgramUniform3ui;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM3UIVPROC              glProgramUniform3uiv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4IPROC                glProgramUniform4i;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4IVPROC               glProgramUniform4iv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4FPROC                glProgramUniform4f;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4FVPROC               glProgramUniform4fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4DPROC                glProgramUniform4d;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4DVPROC               glProgramUniform4dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4UIPROC               glProgramUniform4ui;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORM4UIVPROC              glProgramUniform4uiv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX2FVPROC         glProgramUniformMatrix2fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX3FVPROC         glProgramUniformMatrix3fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX4FVPROC         glProgramUniformMatrix4fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX2DVPROC         glProgramUniformMatrix2dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX3DVPROC         glProgramUniformMatrix3dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX4DVPROC         glProgramUniformMatrix4dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC       glProgramUniformMatrix2x3fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC       glProgramUniformMatrix3x2fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC       glProgramUniformMatrix2x4fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC       glProgramUniformMatrix4x2fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC       glProgramUniformMatrix3x4fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC       glProgramUniformMatrix4x3fv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC       glProgramUniformMatrix2x3dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC       glProgramUniformMatrix3x2dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC       glProgramUniformMatrix2x4dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC       glProgramUniformMatrix4x2dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC       glProgramUniformMatrix3x4dv;
extern LL_THREAD_LOCAL_GL PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC       glProgramUniformMatrix4x3dv;
extern LL_THREAD_LOCAL_GL PFNGLVALIDATEPROGRAMPIPELINEPROC         glValidateProgramPipeline;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMPIPELINEINFOLOGPROC       glGetProgramPipelineInfoLog;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL1DPROC                 glVertexAttribL1d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL2DPROC                 glVertexAttribL2d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL3DPROC                 glVertexAttribL3d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL4DPROC                 glVertexAttribL4d;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL1DVPROC                glVertexAttribL1dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL2DVPROC                glVertexAttribL2dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL3DVPROC                glVertexAttribL3dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBL4DVPROC                glVertexAttribL4dv;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBLPOINTERPROC            glVertexAttribLPointer;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXATTRIBLDVPROC              glGetVertexAttribLdv;
extern LL_THREAD_LOCAL_GL PFNGLVIEWPORTARRAYVPROC                  glViewportArrayv;
extern LL_THREAD_LOCAL_GL PFNGLVIEWPORTINDEXEDFPROC                glViewportIndexedf;
extern LL_THREAD_LOCAL_GL PFNGLVIEWPORTINDEXEDFVPROC               glViewportIndexedfv;
extern LL_THREAD_LOCAL_GL PFNGLSCISSORARRAYVPROC                   glScissorArrayv;
extern LL_THREAD_LOCAL_GL PFNGLSCISSORINDEXEDPROC                  glScissorIndexed;
extern LL_THREAD_LOCAL_GL PFNGLSCISSORINDEXEDVPROC                 glScissorIndexedv;
extern LL_THREAD_LOCAL_GL PFNGLDEPTHRANGEARRAYVPROC                glDepthRangeArrayv;
extern LL_THREAD_LOCAL_GL PFNGLDEPTHRANGEINDEXEDPROC               glDepthRangeIndexed;
extern LL_THREAD_LOCAL_GL PFNGLGETFLOATI_VPROC                     glGetFloati_v;
extern LL_THREAD_LOCAL_GL PFNGLGETDOUBLEI_VPROC                    glGetDoublei_v;

// GL_VERSION_4_2
extern LL_THREAD_LOCAL_GL PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC             glDrawArraysInstancedBaseInstance;
extern LL_THREAD_LOCAL_GL PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC           glDrawElementsInstancedBaseInstance;
extern LL_THREAD_LOCAL_GL PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC glDrawElementsInstancedBaseVertexBaseInstance;
extern LL_THREAD_LOCAL_GL PFNGLGETINTERNALFORMATIVPROC                         glGetInternalformativ;
extern LL_THREAD_LOCAL_GL PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC              glGetActiveAtomicCounterBufferiv;
extern LL_THREAD_LOCAL_GL PFNGLBINDIMAGETEXTUREPROC                            glBindImageTexture;
extern LL_THREAD_LOCAL_GL PFNGLMEMORYBARRIERPROC                               glMemoryBarrier;
extern LL_THREAD_LOCAL_GL PFNGLTEXSTORAGE1DPROC                                glTexStorage1D;
extern LL_THREAD_LOCAL_GL PFNGLTEXSTORAGE2DPROC                                glTexStorage2D;
extern LL_THREAD_LOCAL_GL PFNGLTEXSTORAGE3DPROC                                glTexStorage3D;
extern LL_THREAD_LOCAL_GL PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC              glDrawTransformFeedbackInstanced;
extern LL_THREAD_LOCAL_GL PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC        glDrawTransformFeedbackStreamInstanced;

// GL_VERSION_4_3
extern LL_THREAD_LOCAL_GL PFNGLCLEARBUFFERDATAPROC                             glClearBufferData;
extern LL_THREAD_LOCAL_GL PFNGLCLEARBUFFERSUBDATAPROC                          glClearBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLDISPATCHCOMPUTEPROC                             glDispatchCompute;
extern LL_THREAD_LOCAL_GL PFNGLDISPATCHCOMPUTEINDIRECTPROC                     glDispatchComputeIndirect;
extern LL_THREAD_LOCAL_GL PFNGLCOPYIMAGESUBDATAPROC                            glCopyImageSubData;
extern LL_THREAD_LOCAL_GL PFNGLFRAMEBUFFERPARAMETERIPROC                       glFramebufferParameteri;
extern LL_THREAD_LOCAL_GL PFNGLGETFRAMEBUFFERPARAMETERIVPROC                   glGetFramebufferParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGETINTERNALFORMATI64VPROC                       glGetInternalformati64v;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATETEXSUBIMAGEPROC                       glInvalidateTexSubImage;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATETEXIMAGEPROC                          glInvalidateTexImage;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATEBUFFERSUBDATAPROC                     glInvalidateBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATEBUFFERDATAPROC                        glInvalidateBufferData;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATEFRAMEBUFFERPROC                       glInvalidateFramebuffer;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATESUBFRAMEBUFFERPROC                    glInvalidateSubFramebuffer;
extern LL_THREAD_LOCAL_GL PFNGLMULTIDRAWARRAYSINDIRECTPROC                     glMultiDrawArraysIndirect;
extern LL_THREAD_LOCAL_GL PFNGLMULTIDRAWELEMENTSINDIRECTPROC                   glMultiDrawElementsIndirect;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMINTERFACEIVPROC                       glGetProgramInterfaceiv;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMRESOURCEINDEXPROC                     glGetProgramResourceIndex;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMRESOURCENAMEPROC                      glGetProgramResourceName;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMRESOURCEIVPROC                        glGetProgramResourceiv;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMRESOURCELOCATIONPROC                  glGetProgramResourceLocation;
extern LL_THREAD_LOCAL_GL PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC             glGetProgramResourceLocationIndex;
extern LL_THREAD_LOCAL_GL PFNGLSHADERSTORAGEBLOCKBINDINGPROC                   glShaderStorageBlockBinding;
extern LL_THREAD_LOCAL_GL PFNGLTEXBUFFERRANGEPROC                              glTexBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLTEXSTORAGE2DMULTISAMPLEPROC                     glTexStorage2DMultisample;
extern LL_THREAD_LOCAL_GL PFNGLTEXSTORAGE3DMULTISAMPLEPROC                     glTexStorage3DMultisample;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREVIEWPROC                                 glTextureView;
extern LL_THREAD_LOCAL_GL PFNGLBINDVERTEXBUFFERPROC                            glBindVertexBuffer;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBFORMATPROC                          glVertexAttribFormat;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBIFORMATPROC                         glVertexAttribIFormat;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBLFORMATPROC                         glVertexAttribLFormat;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXATTRIBBINDINGPROC                         glVertexAttribBinding;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXBINDINGDIVISORPROC                        glVertexBindingDivisor;
extern LL_THREAD_LOCAL_GL PFNGLDEBUGMESSAGECONTROLPROC                         glDebugMessageControl;
extern LL_THREAD_LOCAL_GL PFNGLDEBUGMESSAGEINSERTPROC                          glDebugMessageInsert;
extern LL_THREAD_LOCAL_GL PFNGLDEBUGMESSAGECALLBACKPROC                        glDebugMessageCallback;
extern LL_THREAD_LOCAL_GL PFNGLGETDEBUGMESSAGELOGPROC                          glGetDebugMessageLog;
extern LL_THREAD_LOCAL_GL PFNGLPUSHDEBUGGROUPPROC                              glPushDebugGroup;
extern LL_THREAD_LOCAL_GL PFNGLPOPDEBUGGROUPPROC                               glPopDebugGroup;
extern LL_THREAD_LOCAL_GL PFNGLOBJECTLABELPROC                                 glObjectLabel;
extern LL_THREAD_LOCAL_GL PFNGLGETOBJECTLABELPROC                              glGetObjectLabel;
extern LL_THREAD_LOCAL_GL PFNGLOBJECTPTRLABELPROC                              glObjectPtrLabel;
extern LL_THREAD_LOCAL_GL PFNGLGETOBJECTPTRLABELPROC                           glGetObjectPtrLabel;

// GL_VERSION_4_4
extern LL_THREAD_LOCAL_GL PFNGLBUFFERSTORAGEPROC       glBufferStorage;
extern LL_THREAD_LOCAL_GL PFNGLCLEARTEXIMAGEPROC       glClearTexImage;
extern LL_THREAD_LOCAL_GL PFNGLCLEARTEXSUBIMAGEPROC    glClearTexSubImage;
extern LL_THREAD_LOCAL_GL PFNGLBINDBUFFERSBASEPROC     glBindBuffersBase;
extern LL_THREAD_LOCAL_GL PFNGLBINDBUFFERSRANGEPROC    glBindBuffersRange;
extern LL_THREAD_LOCAL_GL PFNGLBINDTEXTURESPROC        glBindTextures;
extern LL_THREAD_LOCAL_GL PFNGLBINDSAMPLERSPROC        glBindSamplers;
extern LL_THREAD_LOCAL_GL PFNGLBINDIMAGETEXTURESPROC   glBindImageTextures;
extern LL_THREAD_LOCAL_GL PFNGLBINDVERTEXBUFFERSPROC   glBindVertexBuffers;

// GL_VERSION_4_5
extern LL_THREAD_LOCAL_GL PFNGLCLIPCONTROLPROC                                     glClipControl;
extern LL_THREAD_LOCAL_GL PFNGLCREATETRANSFORMFEEDBACKSPROC                        glCreateTransformFeedbacks;
extern LL_THREAD_LOCAL_GL PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC                     glTransformFeedbackBufferBase;
extern LL_THREAD_LOCAL_GL PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC                    glTransformFeedbackBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLGETTRANSFORMFEEDBACKIVPROC                          glGetTransformFeedbackiv;
extern LL_THREAD_LOCAL_GL PFNGLGETTRANSFORMFEEDBACKI_VPROC                         glGetTransformFeedbacki_v;
extern LL_THREAD_LOCAL_GL PFNGLGETTRANSFORMFEEDBACKI64_VPROC                       glGetTransformFeedbacki64_v;
extern LL_THREAD_LOCAL_GL PFNGLCREATEBUFFERSPROC                                   glCreateBuffers;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDBUFFERSTORAGEPROC                              glNamedBufferStorage;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDBUFFERDATAPROC                                 glNamedBufferData;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDBUFFERSUBDATAPROC                              glNamedBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLCOPYNAMEDBUFFERSUBDATAPROC                          glCopyNamedBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLCLEARNAMEDBUFFERDATAPROC                            glClearNamedBufferData;
extern LL_THREAD_LOCAL_GL PFNGLCLEARNAMEDBUFFERSUBDATAPROC                         glClearNamedBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLMAPNAMEDBUFFERPROC                                  glMapNamedBuffer;
extern LL_THREAD_LOCAL_GL PFNGLMAPNAMEDBUFFERRANGEPROC                             glMapNamedBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLUNMAPNAMEDBUFFERPROC                                glUnmapNamedBuffer;
extern LL_THREAD_LOCAL_GL PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC                     glFlushMappedNamedBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLGETNAMEDBUFFERPARAMETERIVPROC                       glGetNamedBufferParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGETNAMEDBUFFERPARAMETERI64VPROC                     glGetNamedBufferParameteri64v;
extern LL_THREAD_LOCAL_GL PFNGLGETNAMEDBUFFERPOINTERVPROC                          glGetNamedBufferPointerv;
extern LL_THREAD_LOCAL_GL PFNGLGETNAMEDBUFFERSUBDATAPROC                           glGetNamedBufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLCREATEFRAMEBUFFERSPROC                              glCreateFramebuffers;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC                    glNamedFramebufferRenderbuffer;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC                      glNamedFramebufferParameteri;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDFRAMEBUFFERTEXTUREPROC                         glNamedFramebufferTexture;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC                    glNamedFramebufferTextureLayer;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC                      glNamedFramebufferDrawBuffer;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC                     glNamedFramebufferDrawBuffers;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC                      glNamedFramebufferReadBuffer;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC                  glInvalidateNamedFramebufferData;
extern LL_THREAD_LOCAL_GL PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC               glInvalidateNamedFramebufferSubData;
extern LL_THREAD_LOCAL_GL PFNGLCLEARNAMEDFRAMEBUFFERIVPROC                         glClearNamedFramebufferiv;
extern LL_THREAD_LOCAL_GL PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC                        glClearNamedFramebufferuiv;
extern LL_THREAD_LOCAL_GL PFNGLCLEARNAMEDFRAMEBUFFERFVPROC                         glClearNamedFramebufferfv;
extern LL_THREAD_LOCAL_GL PFNGLCLEARNAMEDFRAMEBUFFERFIPROC                         glClearNamedFramebufferfi;
extern LL_THREAD_LOCAL_GL PFNGLBLITNAMEDFRAMEBUFFERPROC                            glBlitNamedFramebuffer;
extern LL_THREAD_LOCAL_GL PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC                     glCheckNamedFramebufferStatus;
extern LL_THREAD_LOCAL_GL PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC                  glGetNamedFramebufferParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC        glGetNamedFramebufferAttachmentParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLCREATERENDERBUFFERSPROC                             glCreateRenderbuffers;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDRENDERBUFFERSTORAGEPROC                        glNamedRenderbufferStorage;
extern LL_THREAD_LOCAL_GL PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC             glNamedRenderbufferStorageMultisample;
extern LL_THREAD_LOCAL_GL PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC                 glGetNamedRenderbufferParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLCREATETEXTURESPROC                                  glCreateTextures;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREBUFFERPROC                                   glTextureBuffer;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREBUFFERRANGEPROC                              glTextureBufferRange;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESTORAGE1DPROC                                glTextureStorage1D;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESTORAGE2DPROC                                glTextureStorage2D;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESTORAGE3DPROC                                glTextureStorage3D;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC                     glTextureStorage2DMultisample;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC                     glTextureStorage3DMultisample;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESUBIMAGE1DPROC                               glTextureSubImage1D;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESUBIMAGE2DPROC                               glTextureSubImage2D;
extern LL_THREAD_LOCAL_GL PFNGLTEXTURESUBIMAGE3DPROC                               glTextureSubImage3D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC                     glCompressedTextureSubImage1D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC                     glCompressedTextureSubImage2D;
extern LL_THREAD_LOCAL_GL PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC                     glCompressedTextureSubImage3D;
extern LL_THREAD_LOCAL_GL PFNGLCOPYTEXTURESUBIMAGE1DPROC                           glCopyTextureSubImage1D;
extern LL_THREAD_LOCAL_GL PFNGLCOPYTEXTURESUBIMAGE2DPROC                           glCopyTextureSubImage2D;
extern LL_THREAD_LOCAL_GL PFNGLCOPYTEXTURESUBIMAGE3DPROC                           glCopyTextureSubImage3D;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREPARAMETERFPROC                               glTextureParameterf;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREPARAMETERFVPROC                              glTextureParameterfv;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREPARAMETERIPROC                               glTextureParameteri;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREPARAMETERIIVPROC                             glTextureParameterIiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREPARAMETERIUIVPROC                            glTextureParameterIuiv;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREPARAMETERIVPROC                              glTextureParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGENERATETEXTUREMIPMAPPROC                           glGenerateTextureMipmap;
extern LL_THREAD_LOCAL_GL PFNGLBINDTEXTUREUNITPROC                                 glBindTextureUnit;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTUREIMAGEPROC                                 glGetTextureImage;
extern LL_THREAD_LOCAL_GL PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC                       glGetCompressedTextureImage;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTURELEVELPARAMETERFVPROC                      glGetTextureLevelParameterfv;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTURELEVELPARAMETERIVPROC                      glGetTextureLevelParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTUREPARAMETERFVPROC                           glGetTextureParameterfv;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTUREPARAMETERIIVPROC                          glGetTextureParameterIiv;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTUREPARAMETERIUIVPROC                         glGetTextureParameterIuiv;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTUREPARAMETERIVPROC                           glGetTextureParameteriv;
extern LL_THREAD_LOCAL_GL PFNGLCREATEVERTEXARRAYSPROC                              glCreateVertexArrays;
extern LL_THREAD_LOCAL_GL PFNGLDISABLEVERTEXARRAYATTRIBPROC                        glDisableVertexArrayAttrib;
extern LL_THREAD_LOCAL_GL PFNGLENABLEVERTEXARRAYATTRIBPROC                         glEnableVertexArrayAttrib;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYELEMENTBUFFERPROC                        glVertexArrayElementBuffer;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYVERTEXBUFFERPROC                         glVertexArrayVertexBuffer;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYVERTEXBUFFERSPROC                        glVertexArrayVertexBuffers;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYATTRIBBINDINGPROC                        glVertexArrayAttribBinding;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYATTRIBFORMATPROC                         glVertexArrayAttribFormat;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYATTRIBIFORMATPROC                        glVertexArrayAttribIFormat;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYATTRIBLFORMATPROC                        glVertexArrayAttribLFormat;
extern LL_THREAD_LOCAL_GL PFNGLVERTEXARRAYBINDINGDIVISORPROC                       glVertexArrayBindingDivisor;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXARRAYIVPROC                                glGetVertexArrayiv;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXARRAYINDEXEDIVPROC                         glGetVertexArrayIndexediv;
extern LL_THREAD_LOCAL_GL PFNGLGETVERTEXARRAYINDEXED64IVPROC                       glGetVertexArrayIndexed64iv;
extern LL_THREAD_LOCAL_GL PFNGLCREATESAMPLERSPROC                                  glCreateSamplers;
extern LL_THREAD_LOCAL_GL PFNGLCREATEPROGRAMPIPELINESPROC                          glCreateProgramPipelines;
extern LL_THREAD_LOCAL_GL PFNGLCREATEQUERIESPROC                                   glCreateQueries;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYBUFFEROBJECTI64VPROC                        glGetQueryBufferObjecti64v;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYBUFFEROBJECTIVPROC                          glGetQueryBufferObjectiv;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYBUFFEROBJECTUI64VPROC                       glGetQueryBufferObjectui64v;
extern LL_THREAD_LOCAL_GL PFNGLGETQUERYBUFFEROBJECTUIVPROC                         glGetQueryBufferObjectuiv;
extern LL_THREAD_LOCAL_GL PFNGLMEMORYBARRIERBYREGIONPROC                           glMemoryBarrierByRegion;
extern LL_THREAD_LOCAL_GL PFNGLGETTEXTURESUBIMAGEPROC                              glGetTextureSubImage;
extern LL_THREAD_LOCAL_GL PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC                    glGetCompressedTextureSubImage;
extern LL_THREAD_LOCAL_GL PFNGLGETGRAPHICSRESETSTATUSPROC                          glGetGraphicsResetStatus;
extern LL_THREAD_LOCAL_GL PFNGLGETNCOMPRESSEDTEXIMAGEPROC                          glGetnCompressedTexImage;
extern LL_THREAD_LOCAL_GL PFNGLGETNTEXIMAGEPROC                                    glGetnTexImage;
extern LL_THREAD_LOCAL_GL PFNGLGETNUNIFORMDVPROC                                   glGetnUniformdv;
extern LL_THREAD_LOCAL_GL PFNGLGETNUNIFORMFVPROC                                   glGetnUniformfv;
extern LL_THREAD_LOCAL_GL PFNGLGETNUNIFORMIVPROC                                   glGetnUniformiv;
extern LL_THREAD_LOCAL_GL PFNGLGETNUNIFORMUIVPROC                                  glGetnUniformuiv;
extern LL_THREAD_LOCAL_GL PFNGLREADNPIXELSPROC                                     glReadnPixels;
extern LL_THREAD_LOCAL_GL PFNGLGETNMAPDVPROC                                       glGetnMapdv;
extern LL_THREAD_LOCAL_GL PFNGLGETNMAPFVPROC                                       glGetnMapfv;
extern LL_THREAD_LOCAL_GL PFNGLGETNMAPIVPROC                                       glGetnMapiv;
extern LL_THREAD_LOCAL_GL PFNGLGETNPIXELMAPFVPROC                                  glGetnPixelMapfv;
extern LL_THREAD_LOCAL_GL PFNGLGETNPIXELMAPUIVPROC                                 glGetnPixelMapuiv;
extern LL_THREAD_LOCAL_GL PFNGLGETNPIXELMAPUSVPROC                                 glGetnPixelMapusv;
extern LL_THREAD_LOCAL_GL PFNGLGETNPOLYGONSTIPPLEPROC                              glGetnPolygonStipple;
extern LL_THREAD_LOCAL_GL PFNGLGETNCOLORTABLEPROC                                  glGetnColorTable;
extern LL_THREAD_LOCAL_GL PFNGLGETNCONVOLUTIONFILTERPROC                           glGetnConvolutionFilter;
extern LL_THREAD_LOCAL_GL PFNGLGETNSEPARABLEFILTERPROC                             glGetnSeparableFilter;
extern LL_THREAD_LOCAL_GL PFNGLGETNHISTOGRAMPROC                                   glGetnHistogram;
extern LL_THREAD_LOCAL_GL PFNGLGETNMINMAXPROC                                      glGetnMinmax;
extern LL_THREAD_LOCAL_GL PFNGLTEXTUREBARRIERPROC                                  glTextureBarrier;

// GL_VERSION_4_6
extern LL_THREAD_LOCAL_GL PFNGLSPECIALIZESHADERPROC                glSpecializeShader;
extern LL_THREAD_LOCAL_GL PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC    glMultiDrawArraysIndirectCount;
extern LL_THREAD_LOCAL_GL PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC  glMultiDrawElementsIndirectCount;
extern LL_THREAD_LOCAL_GL PFNGLPOLYGONOFFSETCLAMPPROC              glPolygonOffsetClamp;


#elif LL_DARWIN
//----------------------------------------------------------------------------
// LL_DARWIN

#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#define GL_EXT_separate_specular_color 1
#define GL_GLEXT_PROTOTYPES
#include "GL/glext.h"

#include "GL/glh_extensions.h"

// These symbols don't exist on 10.3.9, so they have to be declared weak.  Redeclaring them here fixes the problem.
// Note that they also must not be called on 10.3.9.  This should be taken care of by a runtime check for the existence of the GL extension.
#include <AvailabilityMacros.h>

//GL_EXT_blend_func_separate
extern void glBlendFuncSeparateEXT(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) ;

// GL_EXT_framebuffer_object
extern GLboolean glIsRenderbufferEXT(GLuint renderbuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glBindRenderbufferEXT(GLenum target, GLuint renderbuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glDeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern GLboolean glIsFramebufferEXT(GLuint framebuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glBindFramebufferEXT(GLenum target, GLuint framebuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGenFramebuffersEXT(GLsizei n, GLuint *framebuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern GLenum glCheckFramebufferStatusEXT(GLenum target) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint *params) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGenerateMipmapEXT(GLenum target) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

#ifndef GL_ARB_framebuffer_object
#define glGenerateMipmap glGenerateMipmapEXT
#define GL_MAX_SAMPLES  0x8D57
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Define map buffer range headers on Mac
//
#ifndef GL_ARB_map_buffer_range
#define GL_MAP_READ_BIT                   0x0001
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT         0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020
#endif

//
// Define multisample headers on Mac
//
#ifndef GL_ARB_texture_multisample
#define GL_SAMPLE_POSITION                0x8E50
#define GL_SAMPLE_MASK                    0x8E51
#define GL_SAMPLE_MASK_VALUE              0x8E52
#define GL_MAX_SAMPLE_MASK_WORDS          0x8E59
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE   0x9101
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY   0x9102
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9103
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#define GL_TEXTURE_SAMPLES                0x9106
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS 0x9107
#define GL_SAMPLER_2D_MULTISAMPLE         0x9108
#define GL_INT_SAMPLER_2D_MULTISAMPLE     0x9109
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY   0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F
#define GL_MAX_INTEGER_SAMPLES            0x9110
#endif

//
// Define vertex buffer object headers on Mac
//
#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE_ARB               0x8765
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY_ARB               0x88EA
#endif



#ifndef GL_ARB_vertex_buffer_object
/* GL types for handling large vertex buffer objects */
typedef intptr_t GLintptr;
typedef intptr_t GLsizeiptr;
#endif


#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
#ifdef GL_GLEXT_FUNCTION_POINTERS
typedef void (* glBindBufferARBProcPtr) (GLenum target, GLuint buffer);
typedef void (* glDeleteBufferARBProcPtr) (GLsizei n, const GLuint *buffers);
typedef void (* glGenBuffersARBProcPtr) (GLsizei n, GLuint *buffers);
typedef GLboolean (* glIsBufferARBProcPtr) (GLuint buffer);
typedef void (* glBufferDataARBProcPtr) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
typedef void (* glBufferSubDataARBProcPtr) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
typedef void (* glGetBufferSubDataARBProcPtr) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
typedef GLvoid* (* glMapBufferARBProcPtr) (GLenum target, GLenum access);   /* Flawfinder: ignore */
typedef GLboolean (* glUnmapBufferARBProcPtr) (GLenum target);
typedef void (* glGetBufferParameterivARBProcPtr) (GLenum target, GLenum pname, GLint *params);
typedef void (* glGetBufferPointervARBProcPtr) (GLenum target, GLenum pname, GLvoid* *params);
#else
extern void glBindBufferARB (GLenum, GLuint);
extern void glDeleteBuffersARB (GLsizei, const GLuint *);
extern void glGenBuffersARB (GLsizei, GLuint *);
extern GLboolean glIsBufferARB (GLuint);
extern void glBufferDataARB (GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
extern void glBufferSubDataARB (GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
extern void glGetBufferSubDataARB (GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *);
extern GLvoid* glMapBufferARB (GLenum, GLenum);
extern GLboolean glUnmapBufferARB (GLenum);
extern void glGetBufferParameterivARB (GLenum, GLenum, GLint *);
extern void glGetBufferPointervARB (GLenum, GLenum, GLvoid* *);
#endif /* GL_GLEXT_FUNCTION_POINTERS */
#endif

#ifndef GL_ARB_texture_rg
#define GL_RG                             0x8227
#define GL_RG_INTEGER                     0x8228
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C
#endif

// May be needed for DARWIN...
// #ifndef GL_ARB_compressed_tex_image
// #define GL_ARB_compressed_tex_image 1
// #ifdef GL_GLEXT_FUNCTION_POINTERS
// typedef void (* glCompressedTexImage1D) (GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexImage2D) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexImage3D) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexSubImage1D) (GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexSubImage2D) (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexSubImage3D) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// typedef void (* glGetCompressedTexImage) (GLenum, GLint, GLvoid*);
// #else
// extern void glCompressedTexImage1D (GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*);
// extern void glCompressedTexImage2D (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// extern void glCompressedTexImage3D (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// extern void glCompressedTexSubImage1D (GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);
// extern void glCompressedTexSubImage2D (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// extern void glCompressedTexSubImage3D (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// extern void glGetCompressedTexImage (GLenum, GLint, GLvoid*);
// #endif /* GL_GLEXT_FUNCTION_POINTERS */
// #endif

#ifdef __cplusplus
}
#endif

#include <OpenGL/gl.h>

#elif LL_LINUX

#define GL_GLEXT_PROTOTYPES
#define GLX_GLEXT_PROTOTYPES

#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glext.h"
#include "GL/glx.h"

// The __APPLE__ kludge is to make glh_extensions.h not symbol-clash horribly
# define __APPLE__
# include "GL/glh_extensions.h"
# undef __APPLE__

// #include <X11/Xlib.h>
// #include <X11/Xutil.h>
#include "GL/glh_extensions.h"

#endif // LL_MESA / LL_WINDOWS / LL_DARWIN

// Even when GL_ARB_depth_clamp is available in the driver, the (correct)
// headers, and therefore GL_DEPTH_CLAMP might not be defined.
// In that case GL_DEPTH_CLAMP_NV should be defined, but why not just
// use the known numeric.
//
// To avoid #ifdef's in the code. Just define this here.
#ifndef GL_DEPTH_CLAMP
// Probably (still) called GL_DEPTH_CLAMP_NV.
#define GL_DEPTH_CLAMP 0x864F
#endif

//GL_NVX_gpu_memory_info constants
#ifndef GL_NVX_gpu_memory_info
#define GL_NVX_gpu_memory_info
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B
#endif

//GL_ATI_meminfo constants
#ifndef GL_ATI_meminfo
#define GL_ATI_meminfo
#define GL_VBO_FREE_MEMORY_ATI                     0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI            0x87FD
#endif

#if defined(TRACY_ENABLE) && LL_PROFILER_ENABLE_TRACY_OPENGL
    #include <tracy/TracyOpenGL.hpp>
#endif


#endif // LL_LLGLHEADERS_H
