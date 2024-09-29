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

// WGL_AMD_gpu_association
extern PFNWGLGETGPUIDSAMDPROC                          wglGetGPUIDsAMD;
extern PFNWGLGETGPUINFOAMDPROC                         wglGetGPUInfoAMD;
extern PFNWGLGETCONTEXTGPUIDAMDPROC                    wglGetContextGPUIDAMD;
extern PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC            wglCreateAssociatedContextAMD;
extern PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC     wglCreateAssociatedContextAttribsAMD;
extern PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC            wglDeleteAssociatedContextAMD;
extern PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC       wglMakeAssociatedContextCurrentAMD;
extern PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC        wglGetCurrentAssociatedContextAMD;
extern PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC             wglBlitContextFramebufferAMD;

// WGL_EXT_swap_control
extern PFNWGLSWAPINTERVALEXTPROC    wglSwapIntervalEXT;
extern PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

// WGL_ARB_create_context
extern PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

// GL_VERSION_1_3
extern PFNGLACTIVETEXTUREPROC               glActiveTexture;
extern PFNGLSAMPLECOVERAGEPROC              glSampleCoverage;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC        glCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC        glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC        glCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC     glCompressedTexSubImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC     glCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC     glCompressedTexSubImage1D;
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC       glGetCompressedTexImage;
extern PFNGLCLIENTACTIVETEXTUREPROC         glClientActiveTexture;
extern PFNGLMULTITEXCOORD1DPROC             glMultiTexCoord1d;
extern PFNGLMULTITEXCOORD1DVPROC            glMultiTexCoord1dv;
extern PFNGLMULTITEXCOORD1FPROC             glMultiTexCoord1f;
extern PFNGLMULTITEXCOORD1FVPROC            glMultiTexCoord1fv;
extern PFNGLMULTITEXCOORD1IPROC             glMultiTexCoord1i;
extern PFNGLMULTITEXCOORD1IVPROC            glMultiTexCoord1iv;
extern PFNGLMULTITEXCOORD1SPROC             glMultiTexCoord1s;
extern PFNGLMULTITEXCOORD1SVPROC            glMultiTexCoord1sv;
extern PFNGLMULTITEXCOORD2DPROC             glMultiTexCoord2d;
extern PFNGLMULTITEXCOORD2DVPROC            glMultiTexCoord2dv;
extern PFNGLMULTITEXCOORD2FPROC             glMultiTexCoord2f;
extern PFNGLMULTITEXCOORD2FVPROC            glMultiTexCoord2fv;
extern PFNGLMULTITEXCOORD2IPROC             glMultiTexCoord2i;
extern PFNGLMULTITEXCOORD2IVPROC            glMultiTexCoord2iv;
extern PFNGLMULTITEXCOORD2SPROC             glMultiTexCoord2s;
extern PFNGLMULTITEXCOORD2SVPROC            glMultiTexCoord2sv;
extern PFNGLMULTITEXCOORD3DPROC             glMultiTexCoord3d;
extern PFNGLMULTITEXCOORD3DVPROC            glMultiTexCoord3dv;
extern PFNGLMULTITEXCOORD3FPROC             glMultiTexCoord3f;
extern PFNGLMULTITEXCOORD3FVPROC            glMultiTexCoord3fv;
extern PFNGLMULTITEXCOORD3IPROC             glMultiTexCoord3i;
extern PFNGLMULTITEXCOORD3IVPROC            glMultiTexCoord3iv;
extern PFNGLMULTITEXCOORD3SPROC             glMultiTexCoord3s;
extern PFNGLMULTITEXCOORD3SVPROC            glMultiTexCoord3sv;
extern PFNGLMULTITEXCOORD4DPROC             glMultiTexCoord4d;
extern PFNGLMULTITEXCOORD4DVPROC            glMultiTexCoord4dv;
extern PFNGLMULTITEXCOORD4FPROC             glMultiTexCoord4f;
extern PFNGLMULTITEXCOORD4FVPROC            glMultiTexCoord4fv;
extern PFNGLMULTITEXCOORD4IPROC             glMultiTexCoord4i;
extern PFNGLMULTITEXCOORD4IVPROC            glMultiTexCoord4iv;
extern PFNGLMULTITEXCOORD4SPROC             glMultiTexCoord4s;
extern PFNGLMULTITEXCOORD4SVPROC            glMultiTexCoord4sv;
extern PFNGLLOADTRANSPOSEMATRIXFPROC        glLoadTransposeMatrixf;
extern PFNGLLOADTRANSPOSEMATRIXDPROC        glLoadTransposeMatrixd;
extern PFNGLMULTTRANSPOSEMATRIXFPROC        glMultTransposeMatrixf;
extern PFNGLMULTTRANSPOSEMATRIXDPROC        glMultTransposeMatrixd;

// GL_VERSION_1_4
extern PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;
extern PFNGLMULTIDRAWARRAYSPROC         glMultiDrawArrays;
extern PFNGLMULTIDRAWELEMENTSPROC       glMultiDrawElements;
extern PFNGLPOINTPARAMETERFPROC         glPointParameterf;
extern PFNGLPOINTPARAMETERFVPROC        glPointParameterfv;
extern PFNGLPOINTPARAMETERIPROC         glPointParameteri;
extern PFNGLPOINTPARAMETERIVPROC        glPointParameteriv;
extern PFNGLFOGCOORDFPROC               glFogCoordf;
extern PFNGLFOGCOORDFVPROC              glFogCoordfv;
extern PFNGLFOGCOORDDPROC               glFogCoordd;
extern PFNGLFOGCOORDDVPROC              glFogCoorddv;
extern PFNGLFOGCOORDPOINTERPROC         glFogCoordPointer;
extern PFNGLSECONDARYCOLOR3BPROC        glSecondaryColor3b;
extern PFNGLSECONDARYCOLOR3BVPROC       glSecondaryColor3bv;
extern PFNGLSECONDARYCOLOR3DPROC        glSecondaryColor3d;
extern PFNGLSECONDARYCOLOR3DVPROC       glSecondaryColor3dv;
extern PFNGLSECONDARYCOLOR3FPROC        glSecondaryColor3f;
extern PFNGLSECONDARYCOLOR3FVPROC       glSecondaryColor3fv;
extern PFNGLSECONDARYCOLOR3IPROC        glSecondaryColor3i;
extern PFNGLSECONDARYCOLOR3IVPROC       glSecondaryColor3iv;
extern PFNGLSECONDARYCOLOR3SPROC        glSecondaryColor3s;
extern PFNGLSECONDARYCOLOR3SVPROC       glSecondaryColor3sv;
extern PFNGLSECONDARYCOLOR3UBPROC       glSecondaryColor3ub;
extern PFNGLSECONDARYCOLOR3UBVPROC      glSecondaryColor3ubv;
extern PFNGLSECONDARYCOLOR3UIPROC       glSecondaryColor3ui;
extern PFNGLSECONDARYCOLOR3UIVPROC      glSecondaryColor3uiv;
extern PFNGLSECONDARYCOLOR3USPROC       glSecondaryColor3us;
extern PFNGLSECONDARYCOLOR3USVPROC      glSecondaryColor3usv;
extern PFNGLSECONDARYCOLORPOINTERPROC   glSecondaryColorPointer;
extern PFNGLWINDOWPOS2DPROC             glWindowPos2d;
extern PFNGLWINDOWPOS2DVPROC            glWindowPos2dv;
extern PFNGLWINDOWPOS2FPROC             glWindowPos2f;
extern PFNGLWINDOWPOS2FVPROC            glWindowPos2fv;
extern PFNGLWINDOWPOS2IPROC             glWindowPos2i;
extern PFNGLWINDOWPOS2IVPROC            glWindowPos2iv;
extern PFNGLWINDOWPOS2SPROC             glWindowPos2s;
extern PFNGLWINDOWPOS2SVPROC            glWindowPos2sv;
extern PFNGLWINDOWPOS3DPROC             glWindowPos3d;
extern PFNGLWINDOWPOS3DVPROC            glWindowPos3dv;
extern PFNGLWINDOWPOS3FPROC             glWindowPos3f;
extern PFNGLWINDOWPOS3FVPROC            glWindowPos3fv;
extern PFNGLWINDOWPOS3IPROC             glWindowPos3i;
extern PFNGLWINDOWPOS3IVPROC            glWindowPos3iv;
extern PFNGLWINDOWPOS3SPROC             glWindowPos3s;
extern PFNGLWINDOWPOS3SVPROC            glWindowPos3sv;

// GL_VERSION_1_5
extern PFNGLGENQUERIESPROC              glGenQueries;
extern PFNGLDELETEQUERIESPROC           glDeleteQueries;
extern PFNGLISQUERYPROC                 glIsQuery;
extern PFNGLBEGINQUERYPROC              glBeginQuery;
extern PFNGLENDQUERYPROC                glEndQuery;
extern PFNGLGETQUERYIVPROC              glGetQueryiv;
extern PFNGLGETQUERYOBJECTIVPROC        glGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUIVPROC       glGetQueryObjectuiv;
extern PFNGLBINDBUFFERPROC              glBindBuffer;
extern PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
extern PFNGLGENBUFFERSPROC              glGenBuffers;
extern PFNGLISBUFFERPROC                glIsBuffer;
extern PFNGLBUFFERDATAPROC              glBufferData;
extern PFNGLBUFFERSUBDATAPROC           glBufferSubData;
extern PFNGLGETBUFFERSUBDATAPROC        glGetBufferSubData;
extern PFNGLMAPBUFFERPROC               glMapBuffer;
extern PFNGLUNMAPBUFFERPROC             glUnmapBuffer;
extern PFNGLGETBUFFERPARAMETERIVPROC    glGetBufferParameteriv;
extern PFNGLGETBUFFERPOINTERVPROC       glGetBufferPointerv;

// GL_VERSION_2_0
extern PFNGLBLENDEQUATIONSEPARATEPROC           glBlendEquationSeparate;
extern PFNGLDRAWBUFFERSPROC                     glDrawBuffers;
extern PFNGLSTENCILOPSEPARATEPROC               glStencilOpSeparate;
extern PFNGLSTENCILFUNCSEPARATEPROC             glStencilFuncSeparate;
extern PFNGLSTENCILMASKSEPARATEPROC             glStencilMaskSeparate;
extern PFNGLATTACHSHADERPROC                    glAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC              glBindAttribLocation;
extern PFNGLCOMPILESHADERPROC                   glCompileShader;
extern PFNGLCREATEPROGRAMPROC                   glCreateProgram;
extern PFNGLCREATESHADERPROC                    glCreateShader;
extern PFNGLDELETEPROGRAMPROC                   glDeleteProgram;
extern PFNGLDELETESHADERPROC                    glDeleteShader;
extern PFNGLDETACHSHADERPROC                    glDetachShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC        glDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC         glEnableVertexAttribArray;
extern PFNGLGETACTIVEATTRIBPROC                 glGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC                glGetActiveUniform;
extern PFNGLGETATTACHEDSHADERSPROC              glGetAttachedShaders;
extern PFNGLGETATTRIBLOCATIONPROC               glGetAttribLocation;
extern PFNGLGETPROGRAMIVPROC                    glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC               glGetProgramInfoLog;
extern PFNGLGETSHADERIVPROC                     glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC                glGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC                 glGetShaderSource;
extern PFNGLGETUNIFORMLOCATIONPROC              glGetUniformLocation;
extern PFNGLGETUNIFORMFVPROC                    glGetUniformfv;
extern PFNGLGETUNIFORMIVPROC                    glGetUniformiv;
extern PFNGLGETVERTEXATTRIBDVPROC               glGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC               glGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC               glGetVertexAttribiv;
extern PFNGLGETVERTEXATTRIBPOINTERVPROC         glGetVertexAttribPointerv;
extern PFNGLISPROGRAMPROC                       glIsProgram;
extern PFNGLISSHADERPROC                        glIsShader;
extern PFNGLLINKPROGRAMPROC                     glLinkProgram;
extern PFNGLSHADERSOURCEPROC                    glShaderSource;
extern PFNGLUSEPROGRAMPROC                      glUseProgram;
extern PFNGLUNIFORM1FPROC                       glUniform1f;
extern PFNGLUNIFORM2FPROC                       glUniform2f;
extern PFNGLUNIFORM3FPROC                       glUniform3f;
extern PFNGLUNIFORM4FPROC                       glUniform4f;
extern PFNGLUNIFORM1IPROC                       glUniform1i;
extern PFNGLUNIFORM2IPROC                       glUniform2i;
extern PFNGLUNIFORM3IPROC                       glUniform3i;
extern PFNGLUNIFORM4IPROC                       glUniform4i;
extern PFNGLUNIFORM1FVPROC                      glUniform1fv;
extern PFNGLUNIFORM2FVPROC                      glUniform2fv;
extern PFNGLUNIFORM3FVPROC                      glUniform3fv;
extern PFNGLUNIFORM4FVPROC                      glUniform4fv;
extern PFNGLUNIFORM1IVPROC                      glUniform1iv;
extern PFNGLUNIFORM2IVPROC                      glUniform2iv;
extern PFNGLUNIFORM3IVPROC                      glUniform3iv;
extern PFNGLUNIFORM4IVPROC                      glUniform4iv;
extern PFNGLUNIFORMMATRIX2FVPROC                glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVPROC                glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC                glUniformMatrix4fv;
extern PFNGLVALIDATEPROGRAMPROC                 glValidateProgram;
extern PFNGLVERTEXATTRIB1DPROC                  glVertexAttrib1d;
extern PFNGLVERTEXATTRIB1DVPROC                 glVertexAttrib1dv;
extern PFNGLVERTEXATTRIB1FPROC                  glVertexAttrib1f;
extern PFNGLVERTEXATTRIB1FVPROC                 glVertexAttrib1fv;
extern PFNGLVERTEXATTRIB1SPROC                  glVertexAttrib1s;
extern PFNGLVERTEXATTRIB1SVPROC                 glVertexAttrib1sv;
extern PFNGLVERTEXATTRIB2DPROC                  glVertexAttrib2d;
extern PFNGLVERTEXATTRIB2DVPROC                 glVertexAttrib2dv;
extern PFNGLVERTEXATTRIB2FPROC                  glVertexAttrib2f;
extern PFNGLVERTEXATTRIB2FVPROC                 glVertexAttrib2fv;
extern PFNGLVERTEXATTRIB2SPROC                  glVertexAttrib2s;
extern PFNGLVERTEXATTRIB2SVPROC                 glVertexAttrib2sv;
extern PFNGLVERTEXATTRIB3DPROC                  glVertexAttrib3d;
extern PFNGLVERTEXATTRIB3DVPROC                 glVertexAttrib3dv;
extern PFNGLVERTEXATTRIB3FPROC                  glVertexAttrib3f;
extern PFNGLVERTEXATTRIB3FVPROC                 glVertexAttrib3fv;
extern PFNGLVERTEXATTRIB3SPROC                  glVertexAttrib3s;
extern PFNGLVERTEXATTRIB3SVPROC                 glVertexAttrib3sv;
extern PFNGLVERTEXATTRIB4NBVPROC                glVertexAttrib4Nbv;
extern PFNGLVERTEXATTRIB4NIVPROC                glVertexAttrib4Niv;
extern PFNGLVERTEXATTRIB4NSVPROC                glVertexAttrib4Nsv;
extern PFNGLVERTEXATTRIB4NUBPROC                glVertexAttrib4Nub;
extern PFNGLVERTEXATTRIB4NUBVPROC               glVertexAttrib4Nubv;
extern PFNGLVERTEXATTRIB4NUIVPROC               glVertexAttrib4Nuiv;
extern PFNGLVERTEXATTRIB4NUSVPROC               glVertexAttrib4Nusv;
extern PFNGLVERTEXATTRIB4BVPROC                 glVertexAttrib4bv;
extern PFNGLVERTEXATTRIB4DPROC                  glVertexAttrib4d;
extern PFNGLVERTEXATTRIB4DVPROC                 glVertexAttrib4dv;
extern PFNGLVERTEXATTRIB4FPROC                  glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC                 glVertexAttrib4fv;
extern PFNGLVERTEXATTRIB4IVPROC                 glVertexAttrib4iv;
extern PFNGLVERTEXATTRIB4SPROC                  glVertexAttrib4s;
extern PFNGLVERTEXATTRIB4SVPROC                 glVertexAttrib4sv;
extern PFNGLVERTEXATTRIB4UBVPROC                glVertexAttrib4ubv;
extern PFNGLVERTEXATTRIB4UIVPROC                glVertexAttrib4uiv;
extern PFNGLVERTEXATTRIB4USVPROC                glVertexAttrib4usv;
extern PFNGLVERTEXATTRIBPOINTERPROC             glVertexAttribPointer;

// GL_VERSION_2_1
extern PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv;
extern PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv;
extern PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv;
extern PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv;
extern PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
extern PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv;

// GL_VERSION_3_0
extern PFNGLCOLORMASKIPROC                              glColorMaski;
extern PFNGLGETBOOLEANI_VPROC                           glGetBooleani_v;
extern PFNGLGETINTEGERI_VPROC                           glGetIntegeri_v;
extern PFNGLENABLEIPROC                                 glEnablei;
extern PFNGLDISABLEIPROC                                glDisablei;
extern PFNGLISENABLEDIPROC                              glIsEnabledi;
extern PFNGLBEGINTRANSFORMFEEDBACKPROC                  glBeginTransformFeedback;
extern PFNGLENDTRANSFORMFEEDBACKPROC                    glEndTransformFeedback;
extern PFNGLBINDBUFFERRANGEPROC                         glBindBufferRange;
extern PFNGLBINDBUFFERBASEPROC                          glBindBufferBase;
extern PFNGLTRANSFORMFEEDBACKVARYINGSPROC               glTransformFeedbackVaryings;
extern PFNGLGETTRANSFORMFEEDBACKVARYINGPROC             glGetTransformFeedbackVarying;
extern PFNGLCLAMPCOLORPROC                              glClampColor;
extern PFNGLBEGINCONDITIONALRENDERPROC                  glBeginConditionalRender;
extern PFNGLENDCONDITIONALRENDERPROC                    glEndConditionalRender;
extern PFNGLVERTEXATTRIBIPOINTERPROC                    glVertexAttribIPointer;
extern PFNGLGETVERTEXATTRIBIIVPROC                      glGetVertexAttribIiv;
extern PFNGLGETVERTEXATTRIBIUIVPROC                     glGetVertexAttribIuiv;
extern PFNGLVERTEXATTRIBI1IPROC                         glVertexAttribI1i;
extern PFNGLVERTEXATTRIBI2IPROC                         glVertexAttribI2i;
extern PFNGLVERTEXATTRIBI3IPROC                         glVertexAttribI3i;
extern PFNGLVERTEXATTRIBI4IPROC                         glVertexAttribI4i;
extern PFNGLVERTEXATTRIBI1UIPROC                        glVertexAttribI1ui;
extern PFNGLVERTEXATTRIBI2UIPROC                        glVertexAttribI2ui;
extern PFNGLVERTEXATTRIBI3UIPROC                        glVertexAttribI3ui;
extern PFNGLVERTEXATTRIBI4UIPROC                        glVertexAttribI4ui;
extern PFNGLVERTEXATTRIBI1IVPROC                        glVertexAttribI1iv;
extern PFNGLVERTEXATTRIBI2IVPROC                        glVertexAttribI2iv;
extern PFNGLVERTEXATTRIBI3IVPROC                        glVertexAttribI3iv;
extern PFNGLVERTEXATTRIBI4IVPROC                        glVertexAttribI4iv;
extern PFNGLVERTEXATTRIBI1UIVPROC                       glVertexAttribI1uiv;
extern PFNGLVERTEXATTRIBI2UIVPROC                       glVertexAttribI2uiv;
extern PFNGLVERTEXATTRIBI3UIVPROC                       glVertexAttribI3uiv;
extern PFNGLVERTEXATTRIBI4UIVPROC                       glVertexAttribI4uiv;
extern PFNGLVERTEXATTRIBI4BVPROC                        glVertexAttribI4bv;
extern PFNGLVERTEXATTRIBI4SVPROC                        glVertexAttribI4sv;
extern PFNGLVERTEXATTRIBI4UBVPROC                       glVertexAttribI4ubv;
extern PFNGLVERTEXATTRIBI4USVPROC                       glVertexAttribI4usv;
extern PFNGLGETUNIFORMUIVPROC                           glGetUniformuiv;
extern PFNGLBINDFRAGDATALOCATIONPROC                    glBindFragDataLocation;
extern PFNGLGETFRAGDATALOCATIONPROC                     glGetFragDataLocation;
extern PFNGLUNIFORM1UIPROC                              glUniform1ui;
extern PFNGLUNIFORM2UIPROC                              glUniform2ui;
extern PFNGLUNIFORM3UIPROC                              glUniform3ui;
extern PFNGLUNIFORM4UIPROC                              glUniform4ui;
extern PFNGLUNIFORM1UIVPROC                             glUniform1uiv;
extern PFNGLUNIFORM2UIVPROC                             glUniform2uiv;
extern PFNGLUNIFORM3UIVPROC                             glUniform3uiv;
extern PFNGLUNIFORM4UIVPROC                             glUniform4uiv;
extern PFNGLTEXPARAMETERIIVPROC                         glTexParameterIiv;
extern PFNGLTEXPARAMETERIUIVPROC                        glTexParameterIuiv;
extern PFNGLGETTEXPARAMETERIIVPROC                      glGetTexParameterIiv;
extern PFNGLGETTEXPARAMETERIUIVPROC                     glGetTexParameterIuiv;
extern PFNGLCLEARBUFFERIVPROC                           glClearBufferiv;
extern PFNGLCLEARBUFFERUIVPROC                          glClearBufferuiv;
extern PFNGLCLEARBUFFERFVPROC                           glClearBufferfv;
extern PFNGLCLEARBUFFERFIPROC                           glClearBufferfi;
extern PFNGLGETSTRINGIPROC                              glGetStringi;
extern PFNGLISRENDERBUFFERPROC                          glIsRenderbuffer;
extern PFNGLBINDRENDERBUFFERPROC                        glBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC                     glDeleteRenderbuffers;
extern PFNGLGENRENDERBUFFERSPROC                        glGenRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC                     glRenderbufferStorage;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC              glGetRenderbufferParameteriv;
extern PFNGLISFRAMEBUFFERPROC                           glIsFramebuffer;
extern PFNGLBINDFRAMEBUFFERPROC                         glBindFramebuffer;
extern PFNGLDELETEFRAMEBUFFERSPROC                      glDeleteFramebuffers;
extern PFNGLGENFRAMEBUFFERSPROC                         glGenFramebuffers;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC                  glCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC                    glFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC                    glFramebufferTexture2D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC                    glFramebufferTexture3D;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC                 glFramebufferRenderbuffer;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC     glGetFramebufferAttachmentParameteriv;
extern PFNGLGENERATEMIPMAPPROC                          glGenerateMipmap;
extern PFNGLBLITFRAMEBUFFERPROC                         glBlitFramebuffer;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC          glRenderbufferStorageMultisample;
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC                 glFramebufferTextureLayer;
extern PFNGLMAPBUFFERRANGEPROC                          glMapBufferRange;
extern PFNGLFLUSHMAPPEDBUFFERRANGEPROC                  glFlushMappedBufferRange;
extern PFNGLBINDVERTEXARRAYPROC                         glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC                      glDeleteVertexArrays;
extern PFNGLGENVERTEXARRAYSPROC                         glGenVertexArrays;
extern PFNGLISVERTEXARRAYPROC                           glIsVertexArray;

// GL_VERSION_3_1
extern PFNGLDRAWARRAYSINSTANCEDPROC         glDrawArraysInstanced;
extern PFNGLDRAWELEMENTSINSTANCEDPROC       glDrawElementsInstanced;
extern PFNGLTEXBUFFERPROC                   glTexBuffer;
extern PFNGLPRIMITIVERESTARTINDEXPROC       glPrimitiveRestartIndex;
extern PFNGLCOPYBUFFERSUBDATAPROC           glCopyBufferSubData;
extern PFNGLGETUNIFORMINDICESPROC           glGetUniformIndices;
extern PFNGLGETACTIVEUNIFORMSIVPROC         glGetActiveUniformsiv;
extern PFNGLGETACTIVEUNIFORMNAMEPROC        glGetActiveUniformName;
extern PFNGLGETUNIFORMBLOCKINDEXPROC        glGetUniformBlockIndex;
extern PFNGLGETACTIVEUNIFORMBLOCKIVPROC     glGetActiveUniformBlockiv;
extern PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC   glGetActiveUniformBlockName;
extern PFNGLUNIFORMBLOCKBINDINGPROC         glUniformBlockBinding;

// GL_VERSION_3_2
extern PFNGLDRAWELEMENTSBASEVERTEXPROC          glDrawElementsBaseVertex;
extern PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC     glDrawRangeElementsBaseVertex;
extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glDrawElementsInstancedBaseVertex;
extern PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC     glMultiDrawElementsBaseVertex;
extern PFNGLPROVOKINGVERTEXPROC                 glProvokingVertex;
extern PFNGLFENCESYNCPROC                       glFenceSync;
extern PFNGLISSYNCPROC                          glIsSync;
extern PFNGLDELETESYNCPROC                      glDeleteSync;
extern PFNGLCLIENTWAITSYNCPROC                  glClientWaitSync;
extern PFNGLWAITSYNCPROC                        glWaitSync;
extern PFNGLGETINTEGER64VPROC                   glGetInteger64v;
extern PFNGLGETSYNCIVPROC                       glGetSynciv;
extern PFNGLGETINTEGER64I_VPROC                 glGetInteger64i_v;
extern PFNGLGETBUFFERPARAMETERI64VPROC          glGetBufferParameteri64v;
extern PFNGLFRAMEBUFFERTEXTUREPROC              glFramebufferTexture;
extern PFNGLTEXIMAGE2DMULTISAMPLEPROC           glTexImage2DMultisample;
extern PFNGLTEXIMAGE3DMULTISAMPLEPROC           glTexImage3DMultisample;
extern PFNGLGETMULTISAMPLEFVPROC                glGetMultisamplefv;
extern PFNGLSAMPLEMASKIPROC                     glSampleMaski;

// GL_VERSION_3_3
extern PFNGLBINDFRAGDATALOCATIONINDEXEDPROC  glBindFragDataLocationIndexed;
extern PFNGLGETFRAGDATAINDEXPROC             glGetFragDataIndex;
extern PFNGLGENSAMPLERSPROC                  glGenSamplers;
extern PFNGLDELETESAMPLERSPROC               glDeleteSamplers;
extern PFNGLISSAMPLERPROC                    glIsSampler;
extern PFNGLBINDSAMPLERPROC                  glBindSampler;
extern PFNGLSAMPLERPARAMETERIPROC            glSamplerParameteri;
extern PFNGLSAMPLERPARAMETERIVPROC           glSamplerParameteriv;
extern PFNGLSAMPLERPARAMETERFPROC            glSamplerParameterf;
extern PFNGLSAMPLERPARAMETERFVPROC           glSamplerParameterfv;
extern PFNGLSAMPLERPARAMETERIIVPROC          glSamplerParameterIiv;
extern PFNGLSAMPLERPARAMETERIUIVPROC         glSamplerParameterIuiv;
extern PFNGLGETSAMPLERPARAMETERIVPROC        glGetSamplerParameteriv;
extern PFNGLGETSAMPLERPARAMETERIIVPROC       glGetSamplerParameterIiv;
extern PFNGLGETSAMPLERPARAMETERFVPROC        glGetSamplerParameterfv;
extern PFNGLGETSAMPLERPARAMETERIUIVPROC      glGetSamplerParameterIuiv;
extern PFNGLQUERYCOUNTERPROC                 glQueryCounter;
extern PFNGLGETQUERYOBJECTI64VPROC           glGetQueryObjecti64v;
extern PFNGLGETQUERYOBJECTUI64VPROC          glGetQueryObjectui64v;
extern PFNGLVERTEXATTRIBDIVISORPROC          glVertexAttribDivisor;
extern PFNGLVERTEXATTRIBP1UIPROC             glVertexAttribP1ui;
extern PFNGLVERTEXATTRIBP1UIVPROC            glVertexAttribP1uiv;
extern PFNGLVERTEXATTRIBP2UIPROC             glVertexAttribP2ui;
extern PFNGLVERTEXATTRIBP2UIVPROC            glVertexAttribP2uiv;
extern PFNGLVERTEXATTRIBP3UIPROC             glVertexAttribP3ui;
extern PFNGLVERTEXATTRIBP3UIVPROC            glVertexAttribP3uiv;
extern PFNGLVERTEXATTRIBP4UIPROC             glVertexAttribP4ui;
extern PFNGLVERTEXATTRIBP4UIVPROC            glVertexAttribP4uiv;
extern PFNGLVERTEXP2UIPROC                   glVertexP2ui;
extern PFNGLVERTEXP2UIVPROC                  glVertexP2uiv;
extern PFNGLVERTEXP3UIPROC                   glVertexP3ui;
extern PFNGLVERTEXP3UIVPROC                  glVertexP3uiv;
extern PFNGLVERTEXP4UIPROC                   glVertexP4ui;
extern PFNGLVERTEXP4UIVPROC                  glVertexP4uiv;
extern PFNGLTEXCOORDP1UIPROC                 glTexCoordP1ui;
extern PFNGLTEXCOORDP1UIVPROC                glTexCoordP1uiv;
extern PFNGLTEXCOORDP2UIPROC                 glTexCoordP2ui;
extern PFNGLTEXCOORDP2UIVPROC                glTexCoordP2uiv;
extern PFNGLTEXCOORDP3UIPROC                 glTexCoordP3ui;
extern PFNGLTEXCOORDP3UIVPROC                glTexCoordP3uiv;
extern PFNGLTEXCOORDP4UIPROC                 glTexCoordP4ui;
extern PFNGLTEXCOORDP4UIVPROC                glTexCoordP4uiv;
extern PFNGLMULTITEXCOORDP1UIPROC            glMultiTexCoordP1ui;
extern PFNGLMULTITEXCOORDP1UIVPROC           glMultiTexCoordP1uiv;
extern PFNGLMULTITEXCOORDP2UIPROC            glMultiTexCoordP2ui;
extern PFNGLMULTITEXCOORDP2UIVPROC           glMultiTexCoordP2uiv;
extern PFNGLMULTITEXCOORDP3UIPROC            glMultiTexCoordP3ui;
extern PFNGLMULTITEXCOORDP3UIVPROC           glMultiTexCoordP3uiv;
extern PFNGLMULTITEXCOORDP4UIPROC            glMultiTexCoordP4ui;
extern PFNGLMULTITEXCOORDP4UIVPROC           glMultiTexCoordP4uiv;
extern PFNGLNORMALP3UIPROC                   glNormalP3ui;
extern PFNGLNORMALP3UIVPROC                  glNormalP3uiv;
extern PFNGLCOLORP3UIPROC                    glColorP3ui;
extern PFNGLCOLORP3UIVPROC                   glColorP3uiv;
extern PFNGLCOLORP4UIPROC                    glColorP4ui;
extern PFNGLCOLORP4UIVPROC                   glColorP4uiv;
extern PFNGLSECONDARYCOLORP3UIPROC           glSecondaryColorP3ui;
extern PFNGLSECONDARYCOLORP3UIVPROC          glSecondaryColorP3uiv;

// GL_VERSION_4_0
extern PFNGLMINSAMPLESHADINGPROC                glMinSampleShading;
extern PFNGLBLENDEQUATIONIPROC                  glBlendEquationi;
extern PFNGLBLENDEQUATIONSEPARATEIPROC          glBlendEquationSeparatei;
extern PFNGLBLENDFUNCIPROC                      glBlendFunci;
extern PFNGLBLENDFUNCSEPARATEIPROC              glBlendFuncSeparatei;
extern PFNGLDRAWARRAYSINDIRECTPROC              glDrawArraysIndirect;
extern PFNGLDRAWELEMENTSINDIRECTPROC            glDrawElementsIndirect;
extern PFNGLUNIFORM1DPROC                       glUniform1d;
extern PFNGLUNIFORM2DPROC                       glUniform2d;
extern PFNGLUNIFORM3DPROC                       glUniform3d;
extern PFNGLUNIFORM4DPROC                       glUniform4d;
extern PFNGLUNIFORM1DVPROC                      glUniform1dv;
extern PFNGLUNIFORM2DVPROC                      glUniform2dv;
extern PFNGLUNIFORM3DVPROC                      glUniform3dv;
extern PFNGLUNIFORM4DVPROC                      glUniform4dv;
extern PFNGLUNIFORMMATRIX2DVPROC                glUniformMatrix2dv;
extern PFNGLUNIFORMMATRIX3DVPROC                glUniformMatrix3dv;
extern PFNGLUNIFORMMATRIX4DVPROC                glUniformMatrix4dv;
extern PFNGLUNIFORMMATRIX2X3DVPROC              glUniformMatrix2x3dv;
extern PFNGLUNIFORMMATRIX2X4DVPROC              glUniformMatrix2x4dv;
extern PFNGLUNIFORMMATRIX3X2DVPROC              glUniformMatrix3x2dv;
extern PFNGLUNIFORMMATRIX3X4DVPROC              glUniformMatrix3x4dv;
extern PFNGLUNIFORMMATRIX4X2DVPROC              glUniformMatrix4x2dv;
extern PFNGLUNIFORMMATRIX4X3DVPROC              glUniformMatrix4x3dv;
extern PFNGLGETUNIFORMDVPROC                    glGetUniformdv;
extern PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC    glGetSubroutineUniformLocation;
extern PFNGLGETSUBROUTINEINDEXPROC              glGetSubroutineIndex;
extern PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC    glGetActiveSubroutineUniformiv;
extern PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC  glGetActiveSubroutineUniformName;
extern PFNGLGETACTIVESUBROUTINENAMEPROC         glGetActiveSubroutineName;
extern PFNGLUNIFORMSUBROUTINESUIVPROC           glUniformSubroutinesuiv;
extern PFNGLGETUNIFORMSUBROUTINEUIVPROC         glGetUniformSubroutineuiv;
extern PFNGLGETPROGRAMSTAGEIVPROC               glGetProgramStageiv;
extern PFNGLPATCHPARAMETERIPROC                 glPatchParameteri;
extern PFNGLPATCHPARAMETERFVPROC                glPatchParameterfv;
extern PFNGLBINDTRANSFORMFEEDBACKPROC           glBindTransformFeedback;
extern PFNGLDELETETRANSFORMFEEDBACKSPROC        glDeleteTransformFeedbacks;
extern PFNGLGENTRANSFORMFEEDBACKSPROC           glGenTransformFeedbacks;
extern PFNGLISTRANSFORMFEEDBACKPROC             glIsTransformFeedback;
extern PFNGLPAUSETRANSFORMFEEDBACKPROC          glPauseTransformFeedback;
extern PFNGLRESUMETRANSFORMFEEDBACKPROC         glResumeTransformFeedback;
extern PFNGLDRAWTRANSFORMFEEDBACKPROC           glDrawTransformFeedback;
extern PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC     glDrawTransformFeedbackStream;
extern PFNGLBEGINQUERYINDEXEDPROC               glBeginQueryIndexed;
extern PFNGLENDQUERYINDEXEDPROC                 glEndQueryIndexed;
extern PFNGLGETQUERYINDEXEDIVPROC               glGetQueryIndexediv;

 // GL_VERSION_4_1
extern PFNGLRELEASESHADERCOMPILERPROC           glReleaseShaderCompiler;
extern PFNGLSHADERBINARYPROC                    glShaderBinary;
extern PFNGLGETSHADERPRECISIONFORMATPROC        glGetShaderPrecisionFormat;
extern PFNGLDEPTHRANGEFPROC                     glDepthRangef;
extern PFNGLCLEARDEPTHFPROC                     glClearDepthf;
extern PFNGLGETPROGRAMBINARYPROC                glGetProgramBinary;
extern PFNGLPROGRAMBINARYPROC                   glProgramBinary;
extern PFNGLPROGRAMPARAMETERIPROC               glProgramParameteri;
extern PFNGLUSEPROGRAMSTAGESPROC                glUseProgramStages;
extern PFNGLACTIVESHADERPROGRAMPROC             glActiveShaderProgram;
extern PFNGLCREATESHADERPROGRAMVPROC            glCreateShaderProgramv;
extern PFNGLBINDPROGRAMPIPELINEPROC             glBindProgramPipeline;
extern PFNGLDELETEPROGRAMPIPELINESPROC          glDeleteProgramPipelines;
extern PFNGLGENPROGRAMPIPELINESPROC             glGenProgramPipelines;
extern PFNGLISPROGRAMPIPELINEPROC               glIsProgramPipeline;
extern PFNGLGETPROGRAMPIPELINEIVPROC            glGetProgramPipelineiv;
extern PFNGLPROGRAMUNIFORM1IPROC                glProgramUniform1i;
extern PFNGLPROGRAMUNIFORM1IVPROC               glProgramUniform1iv;
extern PFNGLPROGRAMUNIFORM1FPROC                glProgramUniform1f;
extern PFNGLPROGRAMUNIFORM1FVPROC               glProgramUniform1fv;
extern PFNGLPROGRAMUNIFORM1DPROC                glProgramUniform1d;
extern PFNGLPROGRAMUNIFORM1DVPROC               glProgramUniform1dv;
extern PFNGLPROGRAMUNIFORM1UIPROC               glProgramUniform1ui;
extern PFNGLPROGRAMUNIFORM1UIVPROC              glProgramUniform1uiv;
extern PFNGLPROGRAMUNIFORM2IPROC                glProgramUniform2i;
extern PFNGLPROGRAMUNIFORM2IVPROC               glProgramUniform2iv;
extern PFNGLPROGRAMUNIFORM2FPROC                glProgramUniform2f;
extern PFNGLPROGRAMUNIFORM2FVPROC               glProgramUniform2fv;
extern PFNGLPROGRAMUNIFORM2DPROC                glProgramUniform2d;
extern PFNGLPROGRAMUNIFORM2DVPROC               glProgramUniform2dv;
extern PFNGLPROGRAMUNIFORM2UIPROC               glProgramUniform2ui;
extern PFNGLPROGRAMUNIFORM2UIVPROC              glProgramUniform2uiv;
extern PFNGLPROGRAMUNIFORM3IPROC                glProgramUniform3i;
extern PFNGLPROGRAMUNIFORM3IVPROC               glProgramUniform3iv;
extern PFNGLPROGRAMUNIFORM3FPROC                glProgramUniform3f;
extern PFNGLPROGRAMUNIFORM3FVPROC               glProgramUniform3fv;
extern PFNGLPROGRAMUNIFORM3DPROC                glProgramUniform3d;
extern PFNGLPROGRAMUNIFORM3DVPROC               glProgramUniform3dv;
extern PFNGLPROGRAMUNIFORM3UIPROC               glProgramUniform3ui;
extern PFNGLPROGRAMUNIFORM3UIVPROC              glProgramUniform3uiv;
extern PFNGLPROGRAMUNIFORM4IPROC                glProgramUniform4i;
extern PFNGLPROGRAMUNIFORM4IVPROC               glProgramUniform4iv;
extern PFNGLPROGRAMUNIFORM4FPROC                glProgramUniform4f;
extern PFNGLPROGRAMUNIFORM4FVPROC               glProgramUniform4fv;
extern PFNGLPROGRAMUNIFORM4DPROC                glProgramUniform4d;
extern PFNGLPROGRAMUNIFORM4DVPROC               glProgramUniform4dv;
extern PFNGLPROGRAMUNIFORM4UIPROC               glProgramUniform4ui;
extern PFNGLPROGRAMUNIFORM4UIVPROC              glProgramUniform4uiv;
extern PFNGLPROGRAMUNIFORMMATRIX2FVPROC         glProgramUniformMatrix2fv;
extern PFNGLPROGRAMUNIFORMMATRIX3FVPROC         glProgramUniformMatrix3fv;
extern PFNGLPROGRAMUNIFORMMATRIX4FVPROC         glProgramUniformMatrix4fv;
extern PFNGLPROGRAMUNIFORMMATRIX2DVPROC         glProgramUniformMatrix2dv;
extern PFNGLPROGRAMUNIFORMMATRIX3DVPROC         glProgramUniformMatrix3dv;
extern PFNGLPROGRAMUNIFORMMATRIX4DVPROC         glProgramUniformMatrix4dv;
extern PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC       glProgramUniformMatrix2x3fv;
extern PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC       glProgramUniformMatrix3x2fv;
extern PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC       glProgramUniformMatrix2x4fv;
extern PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC       glProgramUniformMatrix4x2fv;
extern PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC       glProgramUniformMatrix3x4fv;
extern PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC       glProgramUniformMatrix4x3fv;
extern PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC       glProgramUniformMatrix2x3dv;
extern PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC       glProgramUniformMatrix3x2dv;
extern PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC       glProgramUniformMatrix2x4dv;
extern PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC       glProgramUniformMatrix4x2dv;
extern PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC       glProgramUniformMatrix3x4dv;
extern PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC       glProgramUniformMatrix4x3dv;
extern PFNGLVALIDATEPROGRAMPIPELINEPROC         glValidateProgramPipeline;
extern PFNGLGETPROGRAMPIPELINEINFOLOGPROC       glGetProgramPipelineInfoLog;
extern PFNGLVERTEXATTRIBL1DPROC                 glVertexAttribL1d;
extern PFNGLVERTEXATTRIBL2DPROC                 glVertexAttribL2d;
extern PFNGLVERTEXATTRIBL3DPROC                 glVertexAttribL3d;
extern PFNGLVERTEXATTRIBL4DPROC                 glVertexAttribL4d;
extern PFNGLVERTEXATTRIBL1DVPROC                glVertexAttribL1dv;
extern PFNGLVERTEXATTRIBL2DVPROC                glVertexAttribL2dv;
extern PFNGLVERTEXATTRIBL3DVPROC                glVertexAttribL3dv;
extern PFNGLVERTEXATTRIBL4DVPROC                glVertexAttribL4dv;
extern PFNGLVERTEXATTRIBLPOINTERPROC            glVertexAttribLPointer;
extern PFNGLGETVERTEXATTRIBLDVPROC              glGetVertexAttribLdv;
extern PFNGLVIEWPORTARRAYVPROC                  glViewportArrayv;
extern PFNGLVIEWPORTINDEXEDFPROC                glViewportIndexedf;
extern PFNGLVIEWPORTINDEXEDFVPROC               glViewportIndexedfv;
extern PFNGLSCISSORARRAYVPROC                   glScissorArrayv;
extern PFNGLSCISSORINDEXEDPROC                  glScissorIndexed;
extern PFNGLSCISSORINDEXEDVPROC                 glScissorIndexedv;
extern PFNGLDEPTHRANGEARRAYVPROC                glDepthRangeArrayv;
extern PFNGLDEPTHRANGEINDEXEDPROC               glDepthRangeIndexed;
extern PFNGLGETFLOATI_VPROC                     glGetFloati_v;
extern PFNGLGETDOUBLEI_VPROC                    glGetDoublei_v;

// GL_VERSION_4_2
extern PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC             glDrawArraysInstancedBaseInstance;
extern PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC           glDrawElementsInstancedBaseInstance;
extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC glDrawElementsInstancedBaseVertexBaseInstance;
extern PFNGLGETINTERNALFORMATIVPROC                         glGetInternalformativ;
extern PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC              glGetActiveAtomicCounterBufferiv;
extern PFNGLBINDIMAGETEXTUREPROC                            glBindImageTexture;
extern PFNGLMEMORYBARRIERPROC                               glMemoryBarrier;
extern PFNGLTEXSTORAGE1DPROC                                glTexStorage1D;
extern PFNGLTEXSTORAGE2DPROC                                glTexStorage2D;
extern PFNGLTEXSTORAGE3DPROC                                glTexStorage3D;
extern PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC              glDrawTransformFeedbackInstanced;
extern PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC        glDrawTransformFeedbackStreamInstanced;

// GL_VERSION_4_3
extern PFNGLCLEARBUFFERDATAPROC                             glClearBufferData;
extern PFNGLCLEARBUFFERSUBDATAPROC                          glClearBufferSubData;
extern PFNGLDISPATCHCOMPUTEPROC                             glDispatchCompute;
extern PFNGLDISPATCHCOMPUTEINDIRECTPROC                     glDispatchComputeIndirect;
extern PFNGLCOPYIMAGESUBDATAPROC                            glCopyImageSubData;
extern PFNGLFRAMEBUFFERPARAMETERIPROC                       glFramebufferParameteri;
extern PFNGLGETFRAMEBUFFERPARAMETERIVPROC                   glGetFramebufferParameteriv;
extern PFNGLGETINTERNALFORMATI64VPROC                       glGetInternalformati64v;
extern PFNGLINVALIDATETEXSUBIMAGEPROC                       glInvalidateTexSubImage;
extern PFNGLINVALIDATETEXIMAGEPROC                          glInvalidateTexImage;
extern PFNGLINVALIDATEBUFFERSUBDATAPROC                     glInvalidateBufferSubData;
extern PFNGLINVALIDATEBUFFERDATAPROC                        glInvalidateBufferData;
extern PFNGLINVALIDATEFRAMEBUFFERPROC                       glInvalidateFramebuffer;
extern PFNGLINVALIDATESUBFRAMEBUFFERPROC                    glInvalidateSubFramebuffer;
extern PFNGLMULTIDRAWARRAYSINDIRECTPROC                     glMultiDrawArraysIndirect;
extern PFNGLMULTIDRAWELEMENTSINDIRECTPROC                   glMultiDrawElementsIndirect;
extern PFNGLGETPROGRAMINTERFACEIVPROC                       glGetProgramInterfaceiv;
extern PFNGLGETPROGRAMRESOURCEINDEXPROC                     glGetProgramResourceIndex;
extern PFNGLGETPROGRAMRESOURCENAMEPROC                      glGetProgramResourceName;
extern PFNGLGETPROGRAMRESOURCEIVPROC                        glGetProgramResourceiv;
extern PFNGLGETPROGRAMRESOURCELOCATIONPROC                  glGetProgramResourceLocation;
extern PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC             glGetProgramResourceLocationIndex;
extern PFNGLSHADERSTORAGEBLOCKBINDINGPROC                   glShaderStorageBlockBinding;
extern PFNGLTEXBUFFERRANGEPROC                              glTexBufferRange;
extern PFNGLTEXSTORAGE2DMULTISAMPLEPROC                     glTexStorage2DMultisample;
extern PFNGLTEXSTORAGE3DMULTISAMPLEPROC                     glTexStorage3DMultisample;
extern PFNGLTEXTUREVIEWPROC                                 glTextureView;
extern PFNGLBINDVERTEXBUFFERPROC                            glBindVertexBuffer;
extern PFNGLVERTEXATTRIBFORMATPROC                          glVertexAttribFormat;
extern PFNGLVERTEXATTRIBIFORMATPROC                         glVertexAttribIFormat;
extern PFNGLVERTEXATTRIBLFORMATPROC                         glVertexAttribLFormat;
extern PFNGLVERTEXATTRIBBINDINGPROC                         glVertexAttribBinding;
extern PFNGLVERTEXBINDINGDIVISORPROC                        glVertexBindingDivisor;
extern PFNGLDEBUGMESSAGECONTROLPROC                         glDebugMessageControl;
extern PFNGLDEBUGMESSAGEINSERTPROC                          glDebugMessageInsert;
extern PFNGLDEBUGMESSAGECALLBACKPROC                        glDebugMessageCallback;
extern PFNGLGETDEBUGMESSAGELOGPROC                          glGetDebugMessageLog;
extern PFNGLPUSHDEBUGGROUPPROC                              glPushDebugGroup;
extern PFNGLPOPDEBUGGROUPPROC                               glPopDebugGroup;
extern PFNGLOBJECTLABELPROC                                 glObjectLabel;
extern PFNGLGETOBJECTLABELPROC                              glGetObjectLabel;
extern PFNGLOBJECTPTRLABELPROC                              glObjectPtrLabel;
extern PFNGLGETOBJECTPTRLABELPROC                           glGetObjectPtrLabel;

// GL_VERSION_4_4
extern PFNGLBUFFERSTORAGEPROC       glBufferStorage;
extern PFNGLCLEARTEXIMAGEPROC       glClearTexImage;
extern PFNGLCLEARTEXSUBIMAGEPROC    glClearTexSubImage;
extern PFNGLBINDBUFFERSBASEPROC     glBindBuffersBase;
extern PFNGLBINDBUFFERSRANGEPROC    glBindBuffersRange;
extern PFNGLBINDTEXTURESPROC        glBindTextures;
extern PFNGLBINDSAMPLERSPROC        glBindSamplers;
extern PFNGLBINDIMAGETEXTURESPROC   glBindImageTextures;
extern PFNGLBINDVERTEXBUFFERSPROC   glBindVertexBuffers;

// GL_VERSION_4_5
extern PFNGLCLIPCONTROLPROC                                     glClipControl;
extern PFNGLCREATETRANSFORMFEEDBACKSPROC                        glCreateTransformFeedbacks;
extern PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC                     glTransformFeedbackBufferBase;
extern PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC                    glTransformFeedbackBufferRange;
extern PFNGLGETTRANSFORMFEEDBACKIVPROC                          glGetTransformFeedbackiv;
extern PFNGLGETTRANSFORMFEEDBACKI_VPROC                         glGetTransformFeedbacki_v;
extern PFNGLGETTRANSFORMFEEDBACKI64_VPROC                       glGetTransformFeedbacki64_v;
extern PFNGLCREATEBUFFERSPROC                                   glCreateBuffers;
extern PFNGLNAMEDBUFFERSTORAGEPROC                              glNamedBufferStorage;
extern PFNGLNAMEDBUFFERDATAPROC                                 glNamedBufferData;
extern PFNGLNAMEDBUFFERSUBDATAPROC                              glNamedBufferSubData;
extern PFNGLCOPYNAMEDBUFFERSUBDATAPROC                          glCopyNamedBufferSubData;
extern PFNGLCLEARNAMEDBUFFERDATAPROC                            glClearNamedBufferData;
extern PFNGLCLEARNAMEDBUFFERSUBDATAPROC                         glClearNamedBufferSubData;
extern PFNGLMAPNAMEDBUFFERPROC                                  glMapNamedBuffer;
extern PFNGLMAPNAMEDBUFFERRANGEPROC                             glMapNamedBufferRange;
extern PFNGLUNMAPNAMEDBUFFERPROC                                glUnmapNamedBuffer;
extern PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC                     glFlushMappedNamedBufferRange;
extern PFNGLGETNAMEDBUFFERPARAMETERIVPROC                       glGetNamedBufferParameteriv;
extern PFNGLGETNAMEDBUFFERPARAMETERI64VPROC                     glGetNamedBufferParameteri64v;
extern PFNGLGETNAMEDBUFFERPOINTERVPROC                          glGetNamedBufferPointerv;
extern PFNGLGETNAMEDBUFFERSUBDATAPROC                           glGetNamedBufferSubData;
extern PFNGLCREATEFRAMEBUFFERSPROC                              glCreateFramebuffers;
extern PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC                    glNamedFramebufferRenderbuffer;
extern PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC                      glNamedFramebufferParameteri;
extern PFNGLNAMEDFRAMEBUFFERTEXTUREPROC                         glNamedFramebufferTexture;
extern PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC                    glNamedFramebufferTextureLayer;
extern PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC                      glNamedFramebufferDrawBuffer;
extern PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC                     glNamedFramebufferDrawBuffers;
extern PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC                      glNamedFramebufferReadBuffer;
extern PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC                  glInvalidateNamedFramebufferData;
extern PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC               glInvalidateNamedFramebufferSubData;
extern PFNGLCLEARNAMEDFRAMEBUFFERIVPROC                         glClearNamedFramebufferiv;
extern PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC                        glClearNamedFramebufferuiv;
extern PFNGLCLEARNAMEDFRAMEBUFFERFVPROC                         glClearNamedFramebufferfv;
extern PFNGLCLEARNAMEDFRAMEBUFFERFIPROC                         glClearNamedFramebufferfi;
extern PFNGLBLITNAMEDFRAMEBUFFERPROC                            glBlitNamedFramebuffer;
extern PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC                     glCheckNamedFramebufferStatus;
extern PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC                  glGetNamedFramebufferParameteriv;
extern PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC        glGetNamedFramebufferAttachmentParameteriv;
extern PFNGLCREATERENDERBUFFERSPROC                             glCreateRenderbuffers;
extern PFNGLNAMEDRENDERBUFFERSTORAGEPROC                        glNamedRenderbufferStorage;
extern PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC             glNamedRenderbufferStorageMultisample;
extern PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC                 glGetNamedRenderbufferParameteriv;
extern PFNGLCREATETEXTURESPROC                                  glCreateTextures;
extern PFNGLTEXTUREBUFFERPROC                                   glTextureBuffer;
extern PFNGLTEXTUREBUFFERRANGEPROC                              glTextureBufferRange;
extern PFNGLTEXTURESTORAGE1DPROC                                glTextureStorage1D;
extern PFNGLTEXTURESTORAGE2DPROC                                glTextureStorage2D;
extern PFNGLTEXTURESTORAGE3DPROC                                glTextureStorage3D;
extern PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC                     glTextureStorage2DMultisample;
extern PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC                     glTextureStorage3DMultisample;
extern PFNGLTEXTURESUBIMAGE1DPROC                               glTextureSubImage1D;
extern PFNGLTEXTURESUBIMAGE2DPROC                               glTextureSubImage2D;
extern PFNGLTEXTURESUBIMAGE3DPROC                               glTextureSubImage3D;
extern PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC                     glCompressedTextureSubImage1D;
extern PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC                     glCompressedTextureSubImage2D;
extern PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC                     glCompressedTextureSubImage3D;
extern PFNGLCOPYTEXTURESUBIMAGE1DPROC                           glCopyTextureSubImage1D;
extern PFNGLCOPYTEXTURESUBIMAGE2DPROC                           glCopyTextureSubImage2D;
extern PFNGLCOPYTEXTURESUBIMAGE3DPROC                           glCopyTextureSubImage3D;
extern PFNGLTEXTUREPARAMETERFPROC                               glTextureParameterf;
extern PFNGLTEXTUREPARAMETERFVPROC                              glTextureParameterfv;
extern PFNGLTEXTUREPARAMETERIPROC                               glTextureParameteri;
extern PFNGLTEXTUREPARAMETERIIVPROC                             glTextureParameterIiv;
extern PFNGLTEXTUREPARAMETERIUIVPROC                            glTextureParameterIuiv;
extern PFNGLTEXTUREPARAMETERIVPROC                              glTextureParameteriv;
extern PFNGLGENERATETEXTUREMIPMAPPROC                           glGenerateTextureMipmap;
extern PFNGLBINDTEXTUREUNITPROC                                 glBindTextureUnit;
extern PFNGLGETTEXTUREIMAGEPROC                                 glGetTextureImage;
extern PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC                       glGetCompressedTextureImage;
extern PFNGLGETTEXTURELEVELPARAMETERFVPROC                      glGetTextureLevelParameterfv;
extern PFNGLGETTEXTURELEVELPARAMETERIVPROC                      glGetTextureLevelParameteriv;
extern PFNGLGETTEXTUREPARAMETERFVPROC                           glGetTextureParameterfv;
extern PFNGLGETTEXTUREPARAMETERIIVPROC                          glGetTextureParameterIiv;
extern PFNGLGETTEXTUREPARAMETERIUIVPROC                         glGetTextureParameterIuiv;
extern PFNGLGETTEXTUREPARAMETERIVPROC                           glGetTextureParameteriv;
extern PFNGLCREATEVERTEXARRAYSPROC                              glCreateVertexArrays;
extern PFNGLDISABLEVERTEXARRAYATTRIBPROC                        glDisableVertexArrayAttrib;
extern PFNGLENABLEVERTEXARRAYATTRIBPROC                         glEnableVertexArrayAttrib;
extern PFNGLVERTEXARRAYELEMENTBUFFERPROC                        glVertexArrayElementBuffer;
extern PFNGLVERTEXARRAYVERTEXBUFFERPROC                         glVertexArrayVertexBuffer;
extern PFNGLVERTEXARRAYVERTEXBUFFERSPROC                        glVertexArrayVertexBuffers;
extern PFNGLVERTEXARRAYATTRIBBINDINGPROC                        glVertexArrayAttribBinding;
extern PFNGLVERTEXARRAYATTRIBFORMATPROC                         glVertexArrayAttribFormat;
extern PFNGLVERTEXARRAYATTRIBIFORMATPROC                        glVertexArrayAttribIFormat;
extern PFNGLVERTEXARRAYATTRIBLFORMATPROC                        glVertexArrayAttribLFormat;
extern PFNGLVERTEXARRAYBINDINGDIVISORPROC                       glVertexArrayBindingDivisor;
extern PFNGLGETVERTEXARRAYIVPROC                                glGetVertexArrayiv;
extern PFNGLGETVERTEXARRAYINDEXEDIVPROC                         glGetVertexArrayIndexediv;
extern PFNGLGETVERTEXARRAYINDEXED64IVPROC                       glGetVertexArrayIndexed64iv;
extern PFNGLCREATESAMPLERSPROC                                  glCreateSamplers;
extern PFNGLCREATEPROGRAMPIPELINESPROC                          glCreateProgramPipelines;
extern PFNGLCREATEQUERIESPROC                                   glCreateQueries;
extern PFNGLGETQUERYBUFFEROBJECTI64VPROC                        glGetQueryBufferObjecti64v;
extern PFNGLGETQUERYBUFFEROBJECTIVPROC                          glGetQueryBufferObjectiv;
extern PFNGLGETQUERYBUFFEROBJECTUI64VPROC                       glGetQueryBufferObjectui64v;
extern PFNGLGETQUERYBUFFEROBJECTUIVPROC                         glGetQueryBufferObjectuiv;
extern PFNGLMEMORYBARRIERBYREGIONPROC                           glMemoryBarrierByRegion;
extern PFNGLGETTEXTURESUBIMAGEPROC                              glGetTextureSubImage;
extern PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC                    glGetCompressedTextureSubImage;
extern PFNGLGETGRAPHICSRESETSTATUSPROC                          glGetGraphicsResetStatus;
extern PFNGLGETNCOMPRESSEDTEXIMAGEPROC                          glGetnCompressedTexImage;
extern PFNGLGETNTEXIMAGEPROC                                    glGetnTexImage;
extern PFNGLGETNUNIFORMDVPROC                                   glGetnUniformdv;
extern PFNGLGETNUNIFORMFVPROC                                   glGetnUniformfv;
extern PFNGLGETNUNIFORMIVPROC                                   glGetnUniformiv;
extern PFNGLGETNUNIFORMUIVPROC                                  glGetnUniformuiv;
extern PFNGLREADNPIXELSPROC                                     glReadnPixels;
extern PFNGLGETNMAPDVPROC                                       glGetnMapdv;
extern PFNGLGETNMAPFVPROC                                       glGetnMapfv;
extern PFNGLGETNMAPIVPROC                                       glGetnMapiv;
extern PFNGLGETNPIXELMAPFVPROC                                  glGetnPixelMapfv;
extern PFNGLGETNPIXELMAPUIVPROC                                 glGetnPixelMapuiv;
extern PFNGLGETNPIXELMAPUSVPROC                                 glGetnPixelMapusv;
extern PFNGLGETNPOLYGONSTIPPLEPROC                              glGetnPolygonStipple;
extern PFNGLGETNCOLORTABLEPROC                                  glGetnColorTable;
extern PFNGLGETNCONVOLUTIONFILTERPROC                           glGetnConvolutionFilter;
extern PFNGLGETNSEPARABLEFILTERPROC                             glGetnSeparableFilter;
extern PFNGLGETNHISTOGRAMPROC                                   glGetnHistogram;
extern PFNGLGETNMINMAXPROC                                      glGetnMinmax;
extern PFNGLTEXTUREBARRIERPROC                                  glTextureBarrier;

// GL_VERSION_4_6
extern PFNGLSPECIALIZESHADERPROC                glSpecializeShader;
extern PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC    glMultiDrawArraysIndirectCount;
extern PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC  glMultiDrawElementsIndirectCount;
extern PFNGLPOLYGONOFFSETCLAMPPROC              glPolygonOffsetClamp;


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
