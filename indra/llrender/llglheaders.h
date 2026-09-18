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

#if LL_WINDOWS || LL_MESA_HEADLESS || LL_SDL_WINDOW || LL_LINUX
 //----------------------------------------------------------------------------
 // LL_WINDOWS || LL_MESA_HEADLESS || LL_SDL_WINDOW || LL_LINUX

#if LL_MESA_HEADLESS
#define GL_GLEXT_PROTOTYPES 1
#else
#define LL_GL_FUNC_POINTER 1
#endif

 // windows gl headers depend on things like APIENTRY, so include windows.
#include "llwin32headers.h"

// quotes so we get libraries/.../GL/ version
#include "GL/glcorearb.h"

#if LL_WINDOWS && !LL_MESA_HEADLESS
#include "GL/wglext.h"
#endif

#if LL_LINUX && LL_X11 && !LL_MESA_HEADLESS
#define GLX_GLXEXT_LEGACY
#define __gl_h_ 1
#include "GL/glx.h"
#include "GL/glxext.h"
#endif

#if LL_LINUX && LL_WAYLAND && !LL_MESA_HEADLESS
#define EGL_EGL_PROTOTYPES 0
#include "EGL/egl.h"
#endif

#elif LL_DARWIN
//----------------------------------------------------------------------------
// LL_DARWIN

#define GL_GLEXT_LEGACY 1
#include <OpenGL/gl3.h>

#define GL_EXT_separate_specular_color 1
#define GL_GLEXT_PROTOTYPES 1
#include "GL/glext.h"

#endif // LL_MESA_HEADLESS / LL_SDL_WINDOW // LL_LINUX / LL_WINDOWS / LL_DARWIN

// GL_NVX_gpu_memory_info constants
#ifndef GL_NVX_gpu_memory_info
#define GL_NVX_gpu_memory_info
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B
#endif

// GL_ATI_meminfo constants
#ifndef GL_ATI_meminfo
#define GL_ATI_meminfo
#define GL_VBO_FREE_MEMORY_ATI                     0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI            0x87FD
#endif

// GL_EXT_texture_sRGB constants
#ifndef GL_EXT_texture_sRGB
#define GL_EXT_texture_sRGB 1
#define GL_SRGB_EXT                       0x8C40
#define GL_SRGB8_EXT                      0x8C41
#define GL_SRGB_ALPHA_EXT                 0x8C42
#define GL_SRGB8_ALPHA8_EXT               0x8C43
#define GL_SLUMINANCE_ALPHA_EXT           0x8C44
#define GL_SLUMINANCE8_ALPHA8_EXT         0x8C45
#define GL_SLUMINANCE_EXT                 0x8C46
#define GL_SLUMINANCE8_EXT                0x8C47
#define GL_COMPRESSED_SRGB_EXT            0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT      0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT      0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT 0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT  0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif /* GL_EXT_texture_sRGB */

// GL_ARB_vertex_buffer_object constants
#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
#define GL_STREAM_DRAW_ARB  0x88E0
#define GL_STREAM_READ_ARB  0x88E1
#define GL_STREAM_COPY_ARB  0x88E2
#define GL_STATIC_DRAW_ARB  0x88E4
#define GL_STATIC_READ_ARB  0x88E5
#define GL_STATIC_COPY_ARB  0x88E6
#define GL_DYNAMIC_DRAW_ARB 0x88E8
#define GL_DYNAMIC_READ_ARB 0x88E9
#define GL_DYNAMIC_COPY_ARB 0x88EA
#endif

// Deprecated OpenGL defines we still use
#define GL_COLOR_INDEX                    0x1900
#define GL_ALPHA                          0x1906
#define GL_ALPHA8                         0x803C
#define GL_LUMINANCE                      0x1909
#define GL_LUMINANCE_ALPHA                0x190A
#define GL_LUMINANCE8                     0x8040
#define GL_LUMINANCE8_ALPHA8              0x8045
#define GL_COMPRESSED_ALPHA               0x84E9
#define GL_COMPRESSED_LUMINANCE           0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA     0x84EB

#if LL_GL_FUNC_POINTER

#if LL_WINDOWS
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

// WGL_ARB_pixel_format
extern PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
extern PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB;
extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;

#endif // LL_WINDOWS

#if LL_LINUX && LL_X11 && !LL_MESA_HEADLESS
// GLX_MESA_query_renderer
extern PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC glXQueryCurrentRendererIntegerMESA;
extern PFNGLXQUERYCURRENTRENDERERSTRINGMESAPROC glXQueryCurrentRendererStringMESA;
extern PFNGLXQUERYRENDERERINTEGERMESAPROC glXQueryRendererIntegerMESA;
extern PFNGLXQUERYRENDERERSTRINGMESAPROC glXQueryRendererStringMESA;
#endif

#if LL_LINUX && LL_WAYLAND &&!LL_MESA_HEADLESS
extern PFNEGLQUERYSTRINGPROC eglQueryString;
#endif

// We get all functions via getProcAddress when using SDL
// GL_VERSION_1_0
extern PFNGLCULLFACEPROC                    glCullFace;
extern PFNGLFRONTFACEPROC                   glFrontFace;
extern PFNGLHINTPROC                        glHint;
extern PFNGLLINEWIDTHPROC                   glLineWidth;
extern PFNGLPOINTSIZEPROC                   glPointSize;
extern PFNGLPOLYGONMODEPROC                 glPolygonMode;
extern PFNGLSCISSORPROC                     glScissor;
extern PFNGLTEXPARAMETERFPROC               glTexParameterf;
extern PFNGLTEXPARAMETERFVPROC              glTexParameterfv;
extern PFNGLTEXPARAMETERIPROC               glTexParameteri;
extern PFNGLTEXPARAMETERIVPROC              glTexParameteriv;
extern PFNGLTEXIMAGE1DPROC                  glTexImage1D;
extern PFNGLTEXIMAGE2DPROC                  glTexImage2D;
extern PFNGLDRAWBUFFERPROC                  glDrawBuffer;
extern PFNGLCLEARPROC                       glClear;
extern PFNGLCLEARCOLORPROC                  glClearColor;
extern PFNGLCLEARSTENCILPROC                glClearStencil;
extern PFNGLCLEARDEPTHPROC                  glClearDepth;
extern PFNGLSTENCILMASKPROC                 glStencilMask;
extern PFNGLCOLORMASKPROC                   glColorMask;
extern PFNGLDEPTHMASKPROC                   glDepthMask;
extern PFNGLDISABLEPROC                     glDisable;
extern PFNGLENABLEPROC                      glEnable;
extern PFNGLFINISHPROC                      glFinish;
extern PFNGLFLUSHPROC                       glFlush;
extern PFNGLBLENDFUNCPROC                   glBlendFunc;
extern PFNGLLOGICOPPROC                     glLogicOp;
extern PFNGLSTENCILFUNCPROC                 glStencilFunc;
extern PFNGLSTENCILOPPROC                   glStencilOp;
extern PFNGLDEPTHFUNCPROC                   glDepthFunc;
extern PFNGLPIXELSTOREFPROC                 glPixelStoref;
extern PFNGLPIXELSTOREIPROC                 glPixelStorei;
extern PFNGLREADBUFFERPROC                  glReadBuffer;
extern PFNGLREADPIXELSPROC                  glReadPixels;
extern PFNGLGETBOOLEANVPROC                 glGetBooleanv;
extern PFNGLGETDOUBLEVPROC                  glGetDoublev;
extern PFNGLGETERRORPROC                    glGetError;
extern PFNGLGETFLOATVPROC                   glGetFloatv;
extern PFNGLGETINTEGERVPROC                 glGetIntegerv;
extern PFNGLGETSTRINGPROC                   glGetString;
extern PFNGLGETTEXIMAGEPROC                 glGetTexImage;
extern PFNGLGETTEXPARAMETERFVPROC           glGetTexParameterfv;
extern PFNGLGETTEXPARAMETERIVPROC           glGetTexParameteriv;
extern PFNGLGETTEXLEVELPARAMETERFVPROC      glGetTexLevelParameterfv;
extern PFNGLGETTEXLEVELPARAMETERIVPROC      glGetTexLevelParameteriv;
extern PFNGLISENABLEDPROC                   glIsEnabled;
extern PFNGLDEPTHRANGEPROC                  glDepthRange;
extern PFNGLVIEWPORTPROC                    glViewport;

// GL_VERSION_1_1
extern PFNGLDRAWARRAYSPROC                  glDrawArrays;
extern PFNGLDRAWELEMENTSPROC                glDrawElements;
extern PFNGLGETPOINTERVPROC                 glGetPointerv;
extern PFNGLPOLYGONOFFSETPROC               glPolygonOffset;
extern PFNGLCOPYTEXIMAGE1DPROC              glCopyTexImage1D;
extern PFNGLCOPYTEXIMAGE2DPROC              glCopyTexImage2D;
extern PFNGLCOPYTEXSUBIMAGE1DPROC           glCopyTexSubImage1D;
extern PFNGLCOPYTEXSUBIMAGE2DPROC           glCopyTexSubImage2D;
extern PFNGLTEXSUBIMAGE1DPROC               glTexSubImage1D;
extern PFNGLTEXSUBIMAGE2DPROC               glTexSubImage2D;
extern PFNGLBINDTEXTUREPROC                 glBindTexture;
extern PFNGLDELETETEXTURESPROC              glDeleteTextures;
extern PFNGLGENTEXTURESPROC                 glGenTextures;
extern PFNGLISTEXTUREPROC                   glIsTexture;

// GL_VERSION_1_2
extern PFNGLDRAWRANGEELEMENTSPROC           glDrawRangeElements;
extern PFNGLTEXIMAGE3DPROC                  glTexImage3D;
extern PFNGLTEXSUBIMAGE3DPROC               glTexSubImage3D;
extern PFNGLCOPYTEXSUBIMAGE3DPROC           glCopyTexSubImage3D;

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

// GL_VERSION_1_4
extern PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate;
extern PFNGLMULTIDRAWARRAYSPROC         glMultiDrawArrays;
extern PFNGLMULTIDRAWELEMENTSPROC       glMultiDrawElements;
extern PFNGLPOINTPARAMETERFPROC         glPointParameterf;
extern PFNGLPOINTPARAMETERFVPROC        glPointParameterfv;
extern PFNGLPOINTPARAMETERIPROC         glPointParameteri;
extern PFNGLPOINTPARAMETERIVPROC        glPointParameteriv;

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
extern PFNGLTEXTUREBARRIERPROC                                  glTextureBarrier;

// GL_VERSION_4_6
extern PFNGLSPECIALIZESHADERPROC                glSpecializeShader;
extern PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC    glMultiDrawArraysIndirectCount;
extern PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC  glMultiDrawElementsIndirectCount;
extern PFNGLPOLYGONOFFSETCLAMPPROC              glPolygonOffsetClamp;

#endif

#if defined(TRACY_ENABLE) && LL_PROFILER_ENABLE_TRACY_OPENGL
    #include <tracy/TracyOpenGL.hpp>
#endif


#endif // LL_LLGLHEADERS_H
