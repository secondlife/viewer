/** 
 * @file llgl.cpp
 * @brief LLGL implementation
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

// This file sets some global GL parameters, and implements some 
// useful functions for GL operations.

#define GLH_EXT_SINGLE_FILE

#include "linden_common.h"

#include "boost/tokenizer.hpp"

#include "llsys.h"

#include "llgl.h"
#include "llglstates.h"
#include "llrender.h"

#include "llerror.h"
#include "llerrorcontrol.h"
#include "llquaternion.h"
#include "llmath.h"
#include "m4math.h"
#include "llstring.h"
#include "llstacktrace.h"

#include "llglheaders.h"
#include "llglslshader.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

#ifdef _DEBUG
//#define GL_STATE_VERIFY
#endif


bool gDebugSession = false;
bool gDebugGLSession = false;
bool gClothRipple = false;
bool gHeadlessClient = false;
bool gNonInteractive = false;
bool gGLActive = false;

static const std::string HEADLESS_VENDOR_STRING("Linden Lab");
static const std::string HEADLESS_RENDERER_STRING("Headless");
static const std::string HEADLESS_VERSION_STRING("1.0");

llofstream gFailLog;

#if GL_ARB_debug_output

#ifndef APIENTRY
#define APIENTRY
#endif

void APIENTRY gl_debug_callback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message,
                                GLvoid* userParam)
{
    /*if (severity != GL_DEBUG_SEVERITY_HIGH &&
        severity != GL_DEBUG_SEVERITY_MEDIUM &&
        severity != GL_DEBUG_SEVERITY_LOW
        )
    { //suppress out-of-spec messages sent by nvidia driver (mostly vertexbuffer hints)
        return;
    }*/

    // list of messages to suppress
    const char* suppress[] =
    {
        "Buffer detailed info:",
        "Program undefined behavior warning: The current GL state uses a sampler (0) that has depth comparisons enabled"
    };

    for (const char* msg : suppress)
    {
        if (strncmp(msg, message, strlen(msg)) == 0)
        {
            return;
        }
    }

    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        LL_WARNS() << "----- GL ERROR --------" << LL_ENDL;
    }
    else
    {
        LL_WARNS() << "----- GL WARNING -------" << LL_ENDL;
    }
    LL_WARNS() << "Type: " << std::hex << type << LL_ENDL;
    LL_WARNS() << "ID: " << std::hex << id << LL_ENDL;
    LL_WARNS() << "Severity: " << std::hex << severity << LL_ENDL;
    LL_WARNS() << "Message: " << message << LL_ENDL;
    LL_WARNS() << "-----------------------" << LL_ENDL;

    GLint vao = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    GLint vbo = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
    GLint vbo_size = 0;
    if (vbo != 0)
    {
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vbo_size);
    }
    GLint ibo = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ibo);
    GLint ibo_size = 0;
    if (ibo != 0)
    {
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &ibo_size);
    }
    GLint ubo = 0;
    glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &ubo);
    GLint ubo_size = 0;
    GLint ubo_immutable = 0;
    if (ubo != 0)
    {
        glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &ubo_size);
        glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_IMMUTABLE_STORAGE, &ubo_immutable);
    }
    
    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        LL_ERRS() << "Halting on GL Error" << LL_ENDL;
    }
}
#endif

void parse_glsl_version(S32& major, S32& minor);

void ll_init_fail_log(std::string filename)
{
	gFailLog.open(filename.c_str());
}


void ll_fail(std::string msg)
{
	
	if (gDebugSession)
	{
		std::vector<std::string> lines;

		gFailLog << LLError::utcTime() << " " << msg << std::endl;

		gFailLog << "Stack Trace:" << std::endl;

		ll_get_stack_trace(lines);
		
		for(size_t i = 0; i < lines.size(); ++i)
		{
			gFailLog << lines[i] << std::endl;
		}

		gFailLog << "End of Stack Trace." << std::endl << std::endl;

		gFailLog.flush();
	}
};

void ll_close_fail_log()
{
	gFailLog.close();
}

LLMatrix4 gGLObliqueProjectionInverse;

#define LL_GL_NAME_POOLING 0

std::list<LLGLUpdate*> LLGLUpdate::sGLQ;

#if (LL_WINDOWS || LL_LINUX)  && !LL_MESA_HEADLESS

#if LL_WINDOWS
// WGL_ARB_create_context
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

// WGL_AMD_gpu_association
PFNWGLGETGPUIDSAMDPROC                          wglGetGPUIDsAMD = nullptr;
PFNWGLGETGPUINFOAMDPROC                         wglGetGPUInfoAMD = nullptr;
PFNWGLGETCONTEXTGPUIDAMDPROC                    wglGetContextGPUIDAMD = nullptr;
PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC            wglCreateAssociatedContextAMD = nullptr;
PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC     wglCreateAssociatedContextAttribsAMD = nullptr;
PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC            wglDeleteAssociatedContextAMD = nullptr;
PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC       wglMakeAssociatedContextCurrentAMD = nullptr;
PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC        wglGetCurrentAssociatedContextAMD = nullptr;
PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC             wglBlitContextFramebufferAMD = nullptr;

// WGL_EXT_swap_control
PFNWGLSWAPINTERVALEXTPROC    wglSwapIntervalEXT = nullptr;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = nullptr;

#endif

// GL_VERSION_1_2
//PFNGLDRAWRANGEELEMENTSPROC  glDrawRangeElements = nullptr;
//PFNGLTEXIMAGE3DPROC         glTexImage3D = nullptr;
//PFNGLTEXSUBIMAGE3DPROC      glTexSubImage3D = nullptr;
//PFNGLCOPYTEXSUBIMAGE3DPROC  glCopyTexSubImage3D = nullptr;

// GL_VERSION_1_3
PFNGLACTIVETEXTUREPROC               glActiveTexture = nullptr;
PFNGLSAMPLECOVERAGEPROC              glSampleCoverage = nullptr;
PFNGLCOMPRESSEDTEXIMAGE3DPROC        glCompressedTexImage3D = nullptr;
PFNGLCOMPRESSEDTEXIMAGE2DPROC        glCompressedTexImage2D = nullptr;
PFNGLCOMPRESSEDTEXIMAGE1DPROC        glCompressedTexImage1D = nullptr;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC     glCompressedTexSubImage3D = nullptr;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC     glCompressedTexSubImage2D = nullptr;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC     glCompressedTexSubImage1D = nullptr;
PFNGLGETCOMPRESSEDTEXIMAGEPROC       glGetCompressedTexImage = nullptr;
PFNGLCLIENTACTIVETEXTUREPROC         glClientActiveTexture = nullptr;
PFNGLMULTITEXCOORD1DPROC             glMultiTexCoord1d = nullptr;
PFNGLMULTITEXCOORD1DVPROC            glMultiTexCoord1dv = nullptr;
PFNGLMULTITEXCOORD1FPROC             glMultiTexCoord1f = nullptr;
PFNGLMULTITEXCOORD1FVPROC            glMultiTexCoord1fv = nullptr;
PFNGLMULTITEXCOORD1IPROC             glMultiTexCoord1i = nullptr;
PFNGLMULTITEXCOORD1IVPROC            glMultiTexCoord1iv = nullptr;
PFNGLMULTITEXCOORD1SPROC             glMultiTexCoord1s = nullptr;
PFNGLMULTITEXCOORD1SVPROC            glMultiTexCoord1sv = nullptr;
PFNGLMULTITEXCOORD2DPROC             glMultiTexCoord2d = nullptr;
PFNGLMULTITEXCOORD2DVPROC            glMultiTexCoord2dv = nullptr;
PFNGLMULTITEXCOORD2FPROC             glMultiTexCoord2f = nullptr;
PFNGLMULTITEXCOORD2FVPROC            glMultiTexCoord2fv = nullptr;
PFNGLMULTITEXCOORD2IPROC             glMultiTexCoord2i = nullptr;
PFNGLMULTITEXCOORD2IVPROC            glMultiTexCoord2iv = nullptr;
PFNGLMULTITEXCOORD2SPROC             glMultiTexCoord2s = nullptr;
PFNGLMULTITEXCOORD2SVPROC            glMultiTexCoord2sv = nullptr;
PFNGLMULTITEXCOORD3DPROC             glMultiTexCoord3d = nullptr;
PFNGLMULTITEXCOORD3DVPROC            glMultiTexCoord3dv = nullptr;
PFNGLMULTITEXCOORD3FPROC             glMultiTexCoord3f = nullptr;
PFNGLMULTITEXCOORD3FVPROC            glMultiTexCoord3fv = nullptr;
PFNGLMULTITEXCOORD3IPROC             glMultiTexCoord3i = nullptr;
PFNGLMULTITEXCOORD3IVPROC            glMultiTexCoord3iv = nullptr;
PFNGLMULTITEXCOORD3SPROC             glMultiTexCoord3s = nullptr;
PFNGLMULTITEXCOORD3SVPROC            glMultiTexCoord3sv = nullptr;
PFNGLMULTITEXCOORD4DPROC             glMultiTexCoord4d = nullptr;
PFNGLMULTITEXCOORD4DVPROC            glMultiTexCoord4dv = nullptr;
PFNGLMULTITEXCOORD4FPROC             glMultiTexCoord4f = nullptr;
PFNGLMULTITEXCOORD4FVPROC            glMultiTexCoord4fv = nullptr;
PFNGLMULTITEXCOORD4IPROC             glMultiTexCoord4i = nullptr;
PFNGLMULTITEXCOORD4IVPROC            glMultiTexCoord4iv = nullptr;
PFNGLMULTITEXCOORD4SPROC             glMultiTexCoord4s = nullptr;
PFNGLMULTITEXCOORD4SVPROC            glMultiTexCoord4sv = nullptr;
PFNGLLOADTRANSPOSEMATRIXFPROC        glLoadTransposeMatrixf = nullptr;
PFNGLLOADTRANSPOSEMATRIXDPROC        glLoadTransposeMatrixd = nullptr;
PFNGLMULTTRANSPOSEMATRIXFPROC        glMultTransposeMatrixf = nullptr;
PFNGLMULTTRANSPOSEMATRIXDPROC        glMultTransposeMatrixd = nullptr;

// GL_VERSION_1_4
PFNGLBLENDFUNCSEPARATEPROC       glBlendFuncSeparate = nullptr;
PFNGLMULTIDRAWARRAYSPROC         glMultiDrawArrays = nullptr;
PFNGLMULTIDRAWELEMENTSPROC       glMultiDrawElements = nullptr;
PFNGLPOINTPARAMETERFPROC         glPointParameterf = nullptr;
PFNGLPOINTPARAMETERFVPROC        glPointParameterfv = nullptr;
PFNGLPOINTPARAMETERIPROC         glPointParameteri = nullptr;
PFNGLPOINTPARAMETERIVPROC        glPointParameteriv = nullptr;
PFNGLFOGCOORDFPROC               glFogCoordf = nullptr;
PFNGLFOGCOORDFVPROC              glFogCoordfv = nullptr;
PFNGLFOGCOORDDPROC               glFogCoordd = nullptr;
PFNGLFOGCOORDDVPROC              glFogCoorddv = nullptr;
PFNGLFOGCOORDPOINTERPROC         glFogCoordPointer = nullptr;
PFNGLSECONDARYCOLOR3BPROC        glSecondaryColor3b = nullptr;
PFNGLSECONDARYCOLOR3BVPROC       glSecondaryColor3bv = nullptr;
PFNGLSECONDARYCOLOR3DPROC        glSecondaryColor3d = nullptr;
PFNGLSECONDARYCOLOR3DVPROC       glSecondaryColor3dv = nullptr;
PFNGLSECONDARYCOLOR3FPROC        glSecondaryColor3f = nullptr;
PFNGLSECONDARYCOLOR3FVPROC       glSecondaryColor3fv = nullptr;
PFNGLSECONDARYCOLOR3IPROC        glSecondaryColor3i = nullptr;
PFNGLSECONDARYCOLOR3IVPROC       glSecondaryColor3iv = nullptr;
PFNGLSECONDARYCOLOR3SPROC        glSecondaryColor3s = nullptr;
PFNGLSECONDARYCOLOR3SVPROC       glSecondaryColor3sv = nullptr;
PFNGLSECONDARYCOLOR3UBPROC       glSecondaryColor3ub = nullptr;
PFNGLSECONDARYCOLOR3UBVPROC      glSecondaryColor3ubv = nullptr;
PFNGLSECONDARYCOLOR3UIPROC       glSecondaryColor3ui = nullptr;
PFNGLSECONDARYCOLOR3UIVPROC      glSecondaryColor3uiv = nullptr;
PFNGLSECONDARYCOLOR3USPROC       glSecondaryColor3us = nullptr;
PFNGLSECONDARYCOLOR3USVPROC      glSecondaryColor3usv = nullptr;
PFNGLSECONDARYCOLORPOINTERPROC   glSecondaryColorPointer = nullptr;
PFNGLWINDOWPOS2DPROC             glWindowPos2d = nullptr;
PFNGLWINDOWPOS2DVPROC            glWindowPos2dv = nullptr;
PFNGLWINDOWPOS2FPROC             glWindowPos2f = nullptr;
PFNGLWINDOWPOS2FVPROC            glWindowPos2fv = nullptr;
PFNGLWINDOWPOS2IPROC             glWindowPos2i = nullptr;
PFNGLWINDOWPOS2IVPROC            glWindowPos2iv = nullptr;
PFNGLWINDOWPOS2SPROC             glWindowPos2s = nullptr;
PFNGLWINDOWPOS2SVPROC            glWindowPos2sv = nullptr;
PFNGLWINDOWPOS3DPROC             glWindowPos3d = nullptr;
PFNGLWINDOWPOS3DVPROC            glWindowPos3dv = nullptr;
PFNGLWINDOWPOS3FPROC             glWindowPos3f = nullptr;
PFNGLWINDOWPOS3FVPROC            glWindowPos3fv = nullptr;
PFNGLWINDOWPOS3IPROC             glWindowPos3i = nullptr;
PFNGLWINDOWPOS3IVPROC            glWindowPos3iv = nullptr;
PFNGLWINDOWPOS3SPROC             glWindowPos3s = nullptr;
PFNGLWINDOWPOS3SVPROC            glWindowPos3sv = nullptr;

// GL_VERSION_1_5
PFNGLGENQUERIESPROC              glGenQueries = nullptr;
PFNGLDELETEQUERIESPROC           glDeleteQueries = nullptr;
PFNGLISQUERYPROC                 glIsQuery = nullptr;
PFNGLBEGINQUERYPROC              glBeginQuery = nullptr;
PFNGLENDQUERYPROC                glEndQuery = nullptr;
PFNGLGETQUERYIVPROC              glGetQueryiv = nullptr;
PFNGLGETQUERYOBJECTIVPROC        glGetQueryObjectiv = nullptr;
PFNGLGETQUERYOBJECTUIVPROC       glGetQueryObjectuiv = nullptr;
PFNGLBINDBUFFERPROC              glBindBuffer = nullptr;
PFNGLDELETEBUFFERSPROC           glDeleteBuffers = nullptr;
PFNGLGENBUFFERSPROC              glGenBuffers = nullptr;
PFNGLISBUFFERPROC                glIsBuffer = nullptr;
PFNGLBUFFERDATAPROC              glBufferData = nullptr;
PFNGLBUFFERSUBDATAPROC           glBufferSubData = nullptr;
PFNGLGETBUFFERSUBDATAPROC        glGetBufferSubData = nullptr;
PFNGLMAPBUFFERPROC               glMapBuffer = nullptr;
PFNGLUNMAPBUFFERPROC             glUnmapBuffer = nullptr;
PFNGLGETBUFFERPARAMETERIVPROC    glGetBufferParameteriv = nullptr;
PFNGLGETBUFFERPOINTERVPROC       glGetBufferPointerv = nullptr;

// GL_VERSION_2_0
PFNGLBLENDEQUATIONSEPARATEPROC           glBlendEquationSeparate = nullptr;
PFNGLDRAWBUFFERSPROC                     glDrawBuffers = nullptr;
PFNGLSTENCILOPSEPARATEPROC               glStencilOpSeparate = nullptr;
PFNGLSTENCILFUNCSEPARATEPROC             glStencilFuncSeparate = nullptr;
PFNGLSTENCILMASKSEPARATEPROC             glStencilMaskSeparate = nullptr;
PFNGLATTACHSHADERPROC                    glAttachShader = nullptr;
PFNGLBINDATTRIBLOCATIONPROC              glBindAttribLocation = nullptr;
PFNGLCOMPILESHADERPROC                   glCompileShader = nullptr;
PFNGLCREATEPROGRAMPROC                   glCreateProgram = nullptr;
PFNGLCREATESHADERPROC                    glCreateShader = nullptr;
PFNGLDELETEPROGRAMPROC                   glDeleteProgram = nullptr;
PFNGLDELETESHADERPROC                    glDeleteShader = nullptr;
PFNGLDETACHSHADERPROC                    glDetachShader = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC        glDisableVertexAttribArray = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC         glEnableVertexAttribArray = nullptr;
PFNGLGETACTIVEATTRIBPROC                 glGetActiveAttrib = nullptr;
PFNGLGETACTIVEUNIFORMPROC                glGetActiveUniform = nullptr;
PFNGLGETATTACHEDSHADERSPROC              glGetAttachedShaders = nullptr;
PFNGLGETATTRIBLOCATIONPROC               glGetAttribLocation = nullptr;
PFNGLGETPROGRAMIVPROC                    glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC               glGetProgramInfoLog = nullptr;
PFNGLGETSHADERIVPROC                     glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC                glGetShaderInfoLog = nullptr;
PFNGLGETSHADERSOURCEPROC                 glGetShaderSource = nullptr;
PFNGLGETUNIFORMLOCATIONPROC              glGetUniformLocation = nullptr;
PFNGLGETUNIFORMFVPROC                    glGetUniformfv = nullptr;
PFNGLGETUNIFORMIVPROC                    glGetUniformiv = nullptr;
PFNGLGETVERTEXATTRIBDVPROC               glGetVertexAttribdv = nullptr;
PFNGLGETVERTEXATTRIBFVPROC               glGetVertexAttribfv = nullptr;
PFNGLGETVERTEXATTRIBIVPROC               glGetVertexAttribiv = nullptr;
PFNGLGETVERTEXATTRIBPOINTERVPROC         glGetVertexAttribPointerv = nullptr;
PFNGLISPROGRAMPROC                       glIsProgram = nullptr;
PFNGLISSHADERPROC                        glIsShader = nullptr;
PFNGLLINKPROGRAMPROC                     glLinkProgram = nullptr;
PFNGLSHADERSOURCEPROC                    glShaderSource = nullptr;
PFNGLUSEPROGRAMPROC                      glUseProgram = nullptr;
PFNGLUNIFORM1FPROC                       glUniform1f = nullptr;
PFNGLUNIFORM2FPROC                       glUniform2f = nullptr;
PFNGLUNIFORM3FPROC                       glUniform3f = nullptr;
PFNGLUNIFORM4FPROC                       glUniform4f = nullptr;
PFNGLUNIFORM1IPROC                       glUniform1i = nullptr;
PFNGLUNIFORM2IPROC                       glUniform2i = nullptr;
PFNGLUNIFORM3IPROC                       glUniform3i = nullptr;
PFNGLUNIFORM4IPROC                       glUniform4i = nullptr;
PFNGLUNIFORM1FVPROC                      glUniform1fv = nullptr;
PFNGLUNIFORM2FVPROC                      glUniform2fv = nullptr;
PFNGLUNIFORM3FVPROC                      glUniform3fv = nullptr;
PFNGLUNIFORM4FVPROC                      glUniform4fv = nullptr;
PFNGLUNIFORM1IVPROC                      glUniform1iv = nullptr;
PFNGLUNIFORM2IVPROC                      glUniform2iv = nullptr;
PFNGLUNIFORM3IVPROC                      glUniform3iv = nullptr;
PFNGLUNIFORM4IVPROC                      glUniform4iv = nullptr;
PFNGLUNIFORMMATRIX2FVPROC                glUniformMatrix2fv = nullptr;
PFNGLUNIFORMMATRIX3FVPROC                glUniformMatrix3fv = nullptr;
PFNGLUNIFORMMATRIX4FVPROC                glUniformMatrix4fv = nullptr;
PFNGLVALIDATEPROGRAMPROC                 glValidateProgram = nullptr;
PFNGLVERTEXATTRIB1DPROC                  glVertexAttrib1d = nullptr;
PFNGLVERTEXATTRIB1DVPROC                 glVertexAttrib1dv = nullptr;
PFNGLVERTEXATTRIB1FPROC                  glVertexAttrib1f = nullptr;
PFNGLVERTEXATTRIB1FVPROC                 glVertexAttrib1fv = nullptr;
PFNGLVERTEXATTRIB1SPROC                  glVertexAttrib1s = nullptr;
PFNGLVERTEXATTRIB1SVPROC                 glVertexAttrib1sv = nullptr;
PFNGLVERTEXATTRIB2DPROC                  glVertexAttrib2d = nullptr;
PFNGLVERTEXATTRIB2DVPROC                 glVertexAttrib2dv = nullptr;
PFNGLVERTEXATTRIB2FPROC                  glVertexAttrib2f = nullptr;
PFNGLVERTEXATTRIB2FVPROC                 glVertexAttrib2fv = nullptr;
PFNGLVERTEXATTRIB2SPROC                  glVertexAttrib2s = nullptr;
PFNGLVERTEXATTRIB2SVPROC                 glVertexAttrib2sv = nullptr;
PFNGLVERTEXATTRIB3DPROC                  glVertexAttrib3d = nullptr;
PFNGLVERTEXATTRIB3DVPROC                 glVertexAttrib3dv = nullptr;
PFNGLVERTEXATTRIB3FPROC                  glVertexAttrib3f = nullptr;
PFNGLVERTEXATTRIB3FVPROC                 glVertexAttrib3fv = nullptr;
PFNGLVERTEXATTRIB3SPROC                  glVertexAttrib3s = nullptr;
PFNGLVERTEXATTRIB3SVPROC                 glVertexAttrib3sv = nullptr;
PFNGLVERTEXATTRIB4NBVPROC                glVertexAttrib4Nbv = nullptr;
PFNGLVERTEXATTRIB4NIVPROC                glVertexAttrib4Niv = nullptr;
PFNGLVERTEXATTRIB4NSVPROC                glVertexAttrib4Nsv = nullptr;
PFNGLVERTEXATTRIB4NUBPROC                glVertexAttrib4Nub = nullptr;
PFNGLVERTEXATTRIB4NUBVPROC               glVertexAttrib4Nubv = nullptr;
PFNGLVERTEXATTRIB4NUIVPROC               glVertexAttrib4Nuiv = nullptr;
PFNGLVERTEXATTRIB4NUSVPROC               glVertexAttrib4Nusv = nullptr;
PFNGLVERTEXATTRIB4BVPROC                 glVertexAttrib4bv = nullptr;
PFNGLVERTEXATTRIB4DPROC                  glVertexAttrib4d = nullptr;
PFNGLVERTEXATTRIB4DVPROC                 glVertexAttrib4dv = nullptr;
PFNGLVERTEXATTRIB4FPROC                  glVertexAttrib4f = nullptr;
PFNGLVERTEXATTRIB4FVPROC                 glVertexAttrib4fv = nullptr;
PFNGLVERTEXATTRIB4IVPROC                 glVertexAttrib4iv = nullptr;
PFNGLVERTEXATTRIB4SPROC                  glVertexAttrib4s = nullptr;
PFNGLVERTEXATTRIB4SVPROC                 glVertexAttrib4sv = nullptr;
PFNGLVERTEXATTRIB4UBVPROC                glVertexAttrib4ubv = nullptr;
PFNGLVERTEXATTRIB4UIVPROC                glVertexAttrib4uiv = nullptr;
PFNGLVERTEXATTRIB4USVPROC                glVertexAttrib4usv = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC             glVertexAttribPointer = nullptr;

// GL_VERSION_2_1
PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv = nullptr;
PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv = nullptr;
PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv = nullptr;
PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv = nullptr;
PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv = nullptr;
PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv = nullptr;

// GL_VERSION_3_0
PFNGLCOLORMASKIPROC                              glColorMaski = nullptr;
PFNGLGETBOOLEANI_VPROC                           glGetBooleani_v = nullptr;
PFNGLGETINTEGERI_VPROC                           glGetIntegeri_v = nullptr;
PFNGLENABLEIPROC                                 glEnablei = nullptr;
PFNGLDISABLEIPROC                                glDisablei = nullptr;
PFNGLISENABLEDIPROC                              glIsEnabledi = nullptr;
PFNGLBEGINTRANSFORMFEEDBACKPROC                  glBeginTransformFeedback = nullptr;
PFNGLENDTRANSFORMFEEDBACKPROC                    glEndTransformFeedback = nullptr;
PFNGLBINDBUFFERRANGEPROC                         glBindBufferRange = nullptr;
PFNGLBINDBUFFERBASEPROC                          glBindBufferBase = nullptr;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC               glTransformFeedbackVaryings = nullptr;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC             glGetTransformFeedbackVarying = nullptr;
PFNGLCLAMPCOLORPROC                              glClampColor = nullptr;
PFNGLBEGINCONDITIONALRENDERPROC                  glBeginConditionalRender = nullptr;
PFNGLENDCONDITIONALRENDERPROC                    glEndConditionalRender = nullptr;
PFNGLVERTEXATTRIBIPOINTERPROC                    glVertexAttribIPointer = nullptr;
PFNGLGETVERTEXATTRIBIIVPROC                      glGetVertexAttribIiv = nullptr;
PFNGLGETVERTEXATTRIBIUIVPROC                     glGetVertexAttribIuiv = nullptr;
PFNGLVERTEXATTRIBI1IPROC                         glVertexAttribI1i = nullptr;
PFNGLVERTEXATTRIBI2IPROC                         glVertexAttribI2i = nullptr;
PFNGLVERTEXATTRIBI3IPROC                         glVertexAttribI3i = nullptr;
PFNGLVERTEXATTRIBI4IPROC                         glVertexAttribI4i = nullptr;
PFNGLVERTEXATTRIBI1UIPROC                        glVertexAttribI1ui = nullptr;
PFNGLVERTEXATTRIBI2UIPROC                        glVertexAttribI2ui = nullptr;
PFNGLVERTEXATTRIBI3UIPROC                        glVertexAttribI3ui = nullptr;
PFNGLVERTEXATTRIBI4UIPROC                        glVertexAttribI4ui = nullptr;
PFNGLVERTEXATTRIBI1IVPROC                        glVertexAttribI1iv = nullptr;
PFNGLVERTEXATTRIBI2IVPROC                        glVertexAttribI2iv = nullptr;
PFNGLVERTEXATTRIBI3IVPROC                        glVertexAttribI3iv = nullptr;
PFNGLVERTEXATTRIBI4IVPROC                        glVertexAttribI4iv = nullptr;
PFNGLVERTEXATTRIBI1UIVPROC                       glVertexAttribI1uiv = nullptr;
PFNGLVERTEXATTRIBI2UIVPROC                       glVertexAttribI2uiv = nullptr;
PFNGLVERTEXATTRIBI3UIVPROC                       glVertexAttribI3uiv = nullptr;
PFNGLVERTEXATTRIBI4UIVPROC                       glVertexAttribI4uiv = nullptr;
PFNGLVERTEXATTRIBI4BVPROC                        glVertexAttribI4bv = nullptr;
PFNGLVERTEXATTRIBI4SVPROC                        glVertexAttribI4sv = nullptr;
PFNGLVERTEXATTRIBI4UBVPROC                       glVertexAttribI4ubv = nullptr;
PFNGLVERTEXATTRIBI4USVPROC                       glVertexAttribI4usv = nullptr;
PFNGLGETUNIFORMUIVPROC                           glGetUniformuiv = nullptr;
PFNGLBINDFRAGDATALOCATIONPROC                    glBindFragDataLocation = nullptr;
PFNGLGETFRAGDATALOCATIONPROC                     glGetFragDataLocation = nullptr;
PFNGLUNIFORM1UIPROC                              glUniform1ui = nullptr;
PFNGLUNIFORM2UIPROC                              glUniform2ui = nullptr;
PFNGLUNIFORM3UIPROC                              glUniform3ui = nullptr;
PFNGLUNIFORM4UIPROC                              glUniform4ui = nullptr;
PFNGLUNIFORM1UIVPROC                             glUniform1uiv = nullptr;
PFNGLUNIFORM2UIVPROC                             glUniform2uiv = nullptr;
PFNGLUNIFORM3UIVPROC                             glUniform3uiv = nullptr;
PFNGLUNIFORM4UIVPROC                             glUniform4uiv = nullptr;
PFNGLTEXPARAMETERIIVPROC                         glTexParameterIiv = nullptr;
PFNGLTEXPARAMETERIUIVPROC                        glTexParameterIuiv = nullptr;
PFNGLGETTEXPARAMETERIIVPROC                      glGetTexParameterIiv = nullptr;
PFNGLGETTEXPARAMETERIUIVPROC                     glGetTexParameterIuiv = nullptr;
PFNGLCLEARBUFFERIVPROC                           glClearBufferiv = nullptr;
PFNGLCLEARBUFFERUIVPROC                          glClearBufferuiv = nullptr;
PFNGLCLEARBUFFERFVPROC                           glClearBufferfv = nullptr;
PFNGLCLEARBUFFERFIPROC                           glClearBufferfi = nullptr;
PFNGLGETSTRINGIPROC                              glGetStringi = nullptr;
PFNGLISRENDERBUFFERPROC                          glIsRenderbuffer = nullptr;
PFNGLBINDRENDERBUFFERPROC                        glBindRenderbuffer = nullptr;
PFNGLDELETERENDERBUFFERSPROC                     glDeleteRenderbuffers = nullptr;
PFNGLGENRENDERBUFFERSPROC                        glGenRenderbuffers = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC                     glRenderbufferStorage = nullptr;
PFNGLGETRENDERBUFFERPARAMETERIVPROC              glGetRenderbufferParameteriv = nullptr;
PFNGLISFRAMEBUFFERPROC                           glIsFramebuffer = nullptr;
PFNGLBINDFRAMEBUFFERPROC                         glBindFramebuffer = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC                      glDeleteFramebuffers = nullptr;
PFNGLGENFRAMEBUFFERSPROC                         glGenFramebuffers = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC                  glCheckFramebufferStatus = nullptr;
PFNGLFRAMEBUFFERTEXTURE1DPROC                    glFramebufferTexture1D = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC                    glFramebufferTexture2D = nullptr;
PFNGLFRAMEBUFFERTEXTURE3DPROC                    glFramebufferTexture3D = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC                 glFramebufferRenderbuffer = nullptr;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC     glGetFramebufferAttachmentParameteriv = nullptr;
PFNGLGENERATEMIPMAPPROC                          glGenerateMipmap = nullptr;
PFNGLBLITFRAMEBUFFERPROC                         glBlitFramebuffer = nullptr;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC          glRenderbufferStorageMultisample = nullptr;
PFNGLFRAMEBUFFERTEXTURELAYERPROC                 glFramebufferTextureLayer = nullptr;
PFNGLMAPBUFFERRANGEPROC                          glMapBufferRange = nullptr;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC                  glFlushMappedBufferRange = nullptr;
PFNGLBINDVERTEXARRAYPROC                         glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC                      glDeleteVertexArrays = nullptr;
PFNGLGENVERTEXARRAYSPROC                         glGenVertexArrays = nullptr;
PFNGLISVERTEXARRAYPROC                           glIsVertexArray = nullptr;

// GL_VERSION_3_1
PFNGLDRAWARRAYSINSTANCEDPROC         glDrawArraysInstanced = nullptr;
PFNGLDRAWELEMENTSINSTANCEDPROC       glDrawElementsInstanced = nullptr;
PFNGLTEXBUFFERPROC                   glTexBuffer = nullptr;
PFNGLPRIMITIVERESTARTINDEXPROC       glPrimitiveRestartIndex = nullptr;
PFNGLCOPYBUFFERSUBDATAPROC           glCopyBufferSubData = nullptr;
PFNGLGETUNIFORMINDICESPROC           glGetUniformIndices = nullptr;
PFNGLGETACTIVEUNIFORMSIVPROC         glGetActiveUniformsiv = nullptr;
PFNGLGETACTIVEUNIFORMNAMEPROC        glGetActiveUniformName = nullptr;
PFNGLGETUNIFORMBLOCKINDEXPROC        glGetUniformBlockIndex = nullptr;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC     glGetActiveUniformBlockiv = nullptr;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC   glGetActiveUniformBlockName = nullptr;
PFNGLUNIFORMBLOCKBINDINGPROC         glUniformBlockBinding = nullptr;

// GL_VERSION_3_2
PFNGLDRAWELEMENTSBASEVERTEXPROC          glDrawElementsBaseVertex = nullptr;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC     glDrawRangeElementsBaseVertex = nullptr;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glDrawElementsInstancedBaseVertex = nullptr;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC     glMultiDrawElementsBaseVertex = nullptr;
PFNGLPROVOKINGVERTEXPROC                 glProvokingVertex = nullptr;
PFNGLFENCESYNCPROC                       glFenceSync = nullptr;
PFNGLISSYNCPROC                          glIsSync = nullptr;
PFNGLDELETESYNCPROC                      glDeleteSync = nullptr;
PFNGLCLIENTWAITSYNCPROC                  glClientWaitSync = nullptr;
PFNGLWAITSYNCPROC                        glWaitSync = nullptr;
PFNGLGETINTEGER64VPROC                   glGetInteger64v = nullptr;
PFNGLGETSYNCIVPROC                       glGetSynciv = nullptr;
PFNGLGETINTEGER64I_VPROC                 glGetInteger64i_v = nullptr;
PFNGLGETBUFFERPARAMETERI64VPROC          glGetBufferParameteri64v = nullptr;
PFNGLFRAMEBUFFERTEXTUREPROC              glFramebufferTexture = nullptr;
PFNGLTEXIMAGE2DMULTISAMPLEPROC           glTexImage2DMultisample = nullptr;
PFNGLTEXIMAGE3DMULTISAMPLEPROC           glTexImage3DMultisample = nullptr;
PFNGLGETMULTISAMPLEFVPROC                glGetMultisamplefv = nullptr;
PFNGLSAMPLEMASKIPROC                     glSampleMaski = nullptr;

// GL_VERSION_3_3
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC  glBindFragDataLocationIndexed = nullptr;
PFNGLGETFRAGDATAINDEXPROC             glGetFragDataIndex = nullptr;
PFNGLGENSAMPLERSPROC                  glGenSamplers = nullptr;
PFNGLDELETESAMPLERSPROC               glDeleteSamplers = nullptr;
PFNGLISSAMPLERPROC                    glIsSampler = nullptr;
PFNGLBINDSAMPLERPROC                  glBindSampler = nullptr;
PFNGLSAMPLERPARAMETERIPROC            glSamplerParameteri = nullptr;
PFNGLSAMPLERPARAMETERIVPROC           glSamplerParameteriv = nullptr;
PFNGLSAMPLERPARAMETERFPROC            glSamplerParameterf = nullptr;
PFNGLSAMPLERPARAMETERFVPROC           glSamplerParameterfv = nullptr;
PFNGLSAMPLERPARAMETERIIVPROC          glSamplerParameterIiv = nullptr;
PFNGLSAMPLERPARAMETERIUIVPROC         glSamplerParameterIuiv = nullptr;
PFNGLGETSAMPLERPARAMETERIVPROC        glGetSamplerParameteriv = nullptr;
PFNGLGETSAMPLERPARAMETERIIVPROC       glGetSamplerParameterIiv = nullptr;
PFNGLGETSAMPLERPARAMETERFVPROC        glGetSamplerParameterfv = nullptr;
PFNGLGETSAMPLERPARAMETERIUIVPROC      glGetSamplerParameterIuiv = nullptr;
PFNGLQUERYCOUNTERPROC                 glQueryCounter = nullptr;
PFNGLGETQUERYOBJECTI64VPROC           glGetQueryObjecti64v = nullptr;
PFNGLGETQUERYOBJECTUI64VPROC          glGetQueryObjectui64v = nullptr;
PFNGLVERTEXATTRIBDIVISORPROC          glVertexAttribDivisor = nullptr;
PFNGLVERTEXATTRIBP1UIPROC             glVertexAttribP1ui = nullptr;
PFNGLVERTEXATTRIBP1UIVPROC            glVertexAttribP1uiv = nullptr;
PFNGLVERTEXATTRIBP2UIPROC             glVertexAttribP2ui = nullptr;
PFNGLVERTEXATTRIBP2UIVPROC            glVertexAttribP2uiv = nullptr;
PFNGLVERTEXATTRIBP3UIPROC             glVertexAttribP3ui = nullptr;
PFNGLVERTEXATTRIBP3UIVPROC            glVertexAttribP3uiv = nullptr;
PFNGLVERTEXATTRIBP4UIPROC             glVertexAttribP4ui = nullptr;
PFNGLVERTEXATTRIBP4UIVPROC            glVertexAttribP4uiv = nullptr;
PFNGLVERTEXP2UIPROC                   glVertexP2ui = nullptr;
PFNGLVERTEXP2UIVPROC                  glVertexP2uiv = nullptr;
PFNGLVERTEXP3UIPROC                   glVertexP3ui = nullptr;
PFNGLVERTEXP3UIVPROC                  glVertexP3uiv = nullptr;
PFNGLVERTEXP4UIPROC                   glVertexP4ui = nullptr;
PFNGLVERTEXP4UIVPROC                  glVertexP4uiv = nullptr;
PFNGLTEXCOORDP1UIPROC                 glTexCoordP1ui = nullptr;
PFNGLTEXCOORDP1UIVPROC                glTexCoordP1uiv = nullptr;
PFNGLTEXCOORDP2UIPROC                 glTexCoordP2ui = nullptr;
PFNGLTEXCOORDP2UIVPROC                glTexCoordP2uiv = nullptr;
PFNGLTEXCOORDP3UIPROC                 glTexCoordP3ui = nullptr;
PFNGLTEXCOORDP3UIVPROC                glTexCoordP3uiv = nullptr;
PFNGLTEXCOORDP4UIPROC                 glTexCoordP4ui = nullptr;
PFNGLTEXCOORDP4UIVPROC                glTexCoordP4uiv = nullptr;
PFNGLMULTITEXCOORDP1UIPROC            glMultiTexCoordP1ui = nullptr;
PFNGLMULTITEXCOORDP1UIVPROC           glMultiTexCoordP1uiv = nullptr;
PFNGLMULTITEXCOORDP2UIPROC            glMultiTexCoordP2ui = nullptr;
PFNGLMULTITEXCOORDP2UIVPROC           glMultiTexCoordP2uiv = nullptr;
PFNGLMULTITEXCOORDP3UIPROC            glMultiTexCoordP3ui = nullptr;
PFNGLMULTITEXCOORDP3UIVPROC           glMultiTexCoordP3uiv = nullptr;
PFNGLMULTITEXCOORDP4UIPROC            glMultiTexCoordP4ui = nullptr;
PFNGLMULTITEXCOORDP4UIVPROC           glMultiTexCoordP4uiv = nullptr;
PFNGLNORMALP3UIPROC                   glNormalP3ui = nullptr;
PFNGLNORMALP3UIVPROC                  glNormalP3uiv = nullptr;
PFNGLCOLORP3UIPROC                    glColorP3ui = nullptr;
PFNGLCOLORP3UIVPROC                   glColorP3uiv = nullptr;
PFNGLCOLORP4UIPROC                    glColorP4ui = nullptr;
PFNGLCOLORP4UIVPROC                   glColorP4uiv = nullptr;
PFNGLSECONDARYCOLORP3UIPROC           glSecondaryColorP3ui = nullptr;
PFNGLSECONDARYCOLORP3UIVPROC          glSecondaryColorP3uiv = nullptr;

// GL_VERSION_4_0
PFNGLMINSAMPLESHADINGPROC                glMinSampleShading = nullptr;
PFNGLBLENDEQUATIONIPROC                  glBlendEquationi = nullptr;
PFNGLBLENDEQUATIONSEPARATEIPROC          glBlendEquationSeparatei = nullptr;
PFNGLBLENDFUNCIPROC                      glBlendFunci = nullptr;
PFNGLBLENDFUNCSEPARATEIPROC              glBlendFuncSeparatei = nullptr;
PFNGLDRAWARRAYSINDIRECTPROC              glDrawArraysIndirect = nullptr;
PFNGLDRAWELEMENTSINDIRECTPROC            glDrawElementsIndirect = nullptr;
PFNGLUNIFORM1DPROC                       glUniform1d = nullptr;
PFNGLUNIFORM2DPROC                       glUniform2d = nullptr;
PFNGLUNIFORM3DPROC                       glUniform3d = nullptr;
PFNGLUNIFORM4DPROC                       glUniform4d = nullptr;
PFNGLUNIFORM1DVPROC                      glUniform1dv = nullptr;
PFNGLUNIFORM2DVPROC                      glUniform2dv = nullptr;
PFNGLUNIFORM3DVPROC                      glUniform3dv = nullptr;
PFNGLUNIFORM4DVPROC                      glUniform4dv = nullptr;
PFNGLUNIFORMMATRIX2DVPROC                glUniformMatrix2dv = nullptr;
PFNGLUNIFORMMATRIX3DVPROC                glUniformMatrix3dv = nullptr;
PFNGLUNIFORMMATRIX4DVPROC                glUniformMatrix4dv = nullptr;
PFNGLUNIFORMMATRIX2X3DVPROC              glUniformMatrix2x3dv = nullptr;
PFNGLUNIFORMMATRIX2X4DVPROC              glUniformMatrix2x4dv = nullptr;
PFNGLUNIFORMMATRIX3X2DVPROC              glUniformMatrix3x2dv = nullptr;
PFNGLUNIFORMMATRIX3X4DVPROC              glUniformMatrix3x4dv = nullptr;
PFNGLUNIFORMMATRIX4X2DVPROC              glUniformMatrix4x2dv = nullptr;
PFNGLUNIFORMMATRIX4X3DVPROC              glUniformMatrix4x3dv = nullptr;
PFNGLGETUNIFORMDVPROC                    glGetUniformdv = nullptr;
PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC    glGetSubroutineUniformLocation = nullptr;
PFNGLGETSUBROUTINEINDEXPROC              glGetSubroutineIndex = nullptr;
PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC    glGetActiveSubroutineUniformiv = nullptr;
PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC  glGetActiveSubroutineUniformName = nullptr;
PFNGLGETACTIVESUBROUTINENAMEPROC         glGetActiveSubroutineName = nullptr;
PFNGLUNIFORMSUBROUTINESUIVPROC           glUniformSubroutinesuiv = nullptr;
PFNGLGETUNIFORMSUBROUTINEUIVPROC         glGetUniformSubroutineuiv = nullptr;
PFNGLGETPROGRAMSTAGEIVPROC               glGetProgramStageiv = nullptr;
PFNGLPATCHPARAMETERIPROC                 glPatchParameteri = nullptr;
PFNGLPATCHPARAMETERFVPROC                glPatchParameterfv = nullptr;
PFNGLBINDTRANSFORMFEEDBACKPROC           glBindTransformFeedback = nullptr;
PFNGLDELETETRANSFORMFEEDBACKSPROC        glDeleteTransformFeedbacks = nullptr;
PFNGLGENTRANSFORMFEEDBACKSPROC           glGenTransformFeedbacks = nullptr;
PFNGLISTRANSFORMFEEDBACKPROC             glIsTransformFeedback = nullptr;
PFNGLPAUSETRANSFORMFEEDBACKPROC          glPauseTransformFeedback = nullptr;
PFNGLRESUMETRANSFORMFEEDBACKPROC         glResumeTransformFeedback = nullptr;
PFNGLDRAWTRANSFORMFEEDBACKPROC           glDrawTransformFeedback = nullptr;
PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC     glDrawTransformFeedbackStream = nullptr;
PFNGLBEGINQUERYINDEXEDPROC               glBeginQueryIndexed = nullptr;
PFNGLENDQUERYINDEXEDPROC                 glEndQueryIndexed = nullptr;
PFNGLGETQUERYINDEXEDIVPROC               glGetQueryIndexediv = nullptr;

// GL_VERSION_4_1
PFNGLRELEASESHADERCOMPILERPROC           glReleaseShaderCompiler = nullptr;
PFNGLSHADERBINARYPROC                    glShaderBinary = nullptr;
PFNGLGETSHADERPRECISIONFORMATPROC        glGetShaderPrecisionFormat = nullptr;
PFNGLDEPTHRANGEFPROC                     glDepthRangef = nullptr;
PFNGLCLEARDEPTHFPROC                     glClearDepthf = nullptr;
PFNGLGETPROGRAMBINARYPROC                glGetProgramBinary = nullptr;
PFNGLPROGRAMBINARYPROC                   glProgramBinary = nullptr;
PFNGLPROGRAMPARAMETERIPROC               glProgramParameteri = nullptr;
PFNGLUSEPROGRAMSTAGESPROC                glUseProgramStages = nullptr;
PFNGLACTIVESHADERPROGRAMPROC             glActiveShaderProgram = nullptr;
PFNGLCREATESHADERPROGRAMVPROC            glCreateShaderProgramv = nullptr;
PFNGLBINDPROGRAMPIPELINEPROC             glBindProgramPipeline = nullptr;
PFNGLDELETEPROGRAMPIPELINESPROC          glDeleteProgramPipelines = nullptr;
PFNGLGENPROGRAMPIPELINESPROC             glGenProgramPipelines = nullptr;
PFNGLISPROGRAMPIPELINEPROC               glIsProgramPipeline = nullptr;
PFNGLGETPROGRAMPIPELINEIVPROC            glGetProgramPipelineiv = nullptr;
PFNGLPROGRAMUNIFORM1IPROC                glProgramUniform1i = nullptr;
PFNGLPROGRAMUNIFORM1IVPROC               glProgramUniform1iv = nullptr;
PFNGLPROGRAMUNIFORM1FPROC                glProgramUniform1f = nullptr;
PFNGLPROGRAMUNIFORM1FVPROC               glProgramUniform1fv = nullptr;
PFNGLPROGRAMUNIFORM1DPROC                glProgramUniform1d = nullptr;
PFNGLPROGRAMUNIFORM1DVPROC               glProgramUniform1dv = nullptr;
PFNGLPROGRAMUNIFORM1UIPROC               glProgramUniform1ui = nullptr;
PFNGLPROGRAMUNIFORM1UIVPROC              glProgramUniform1uiv = nullptr;
PFNGLPROGRAMUNIFORM2IPROC                glProgramUniform2i = nullptr;
PFNGLPROGRAMUNIFORM2IVPROC               glProgramUniform2iv = nullptr;
PFNGLPROGRAMUNIFORM2FPROC                glProgramUniform2f = nullptr;
PFNGLPROGRAMUNIFORM2FVPROC               glProgramUniform2fv = nullptr;
PFNGLPROGRAMUNIFORM2DPROC                glProgramUniform2d = nullptr;
PFNGLPROGRAMUNIFORM2DVPROC               glProgramUniform2dv = nullptr;
PFNGLPROGRAMUNIFORM2UIPROC               glProgramUniform2ui = nullptr;
PFNGLPROGRAMUNIFORM2UIVPROC              glProgramUniform2uiv = nullptr;
PFNGLPROGRAMUNIFORM3IPROC                glProgramUniform3i = nullptr;
PFNGLPROGRAMUNIFORM3IVPROC               glProgramUniform3iv = nullptr;
PFNGLPROGRAMUNIFORM3FPROC                glProgramUniform3f = nullptr;
PFNGLPROGRAMUNIFORM3FVPROC               glProgramUniform3fv = nullptr;
PFNGLPROGRAMUNIFORM3DPROC                glProgramUniform3d = nullptr;
PFNGLPROGRAMUNIFORM3DVPROC               glProgramUniform3dv = nullptr;
PFNGLPROGRAMUNIFORM3UIPROC               glProgramUniform3ui = nullptr;
PFNGLPROGRAMUNIFORM3UIVPROC              glProgramUniform3uiv = nullptr;
PFNGLPROGRAMUNIFORM4IPROC                glProgramUniform4i = nullptr;
PFNGLPROGRAMUNIFORM4IVPROC               glProgramUniform4iv = nullptr;
PFNGLPROGRAMUNIFORM4FPROC                glProgramUniform4f = nullptr;
PFNGLPROGRAMUNIFORM4FVPROC               glProgramUniform4fv = nullptr;
PFNGLPROGRAMUNIFORM4DPROC                glProgramUniform4d = nullptr;
PFNGLPROGRAMUNIFORM4DVPROC               glProgramUniform4dv = nullptr;
PFNGLPROGRAMUNIFORM4UIPROC               glProgramUniform4ui = nullptr;
PFNGLPROGRAMUNIFORM4UIVPROC              glProgramUniform4uiv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX2FVPROC         glProgramUniformMatrix2fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX3FVPROC         glProgramUniformMatrix3fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX4FVPROC         glProgramUniformMatrix4fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX2DVPROC         glProgramUniformMatrix2dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX3DVPROC         glProgramUniformMatrix3dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX4DVPROC         glProgramUniformMatrix4dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC       glProgramUniformMatrix2x3fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC       glProgramUniformMatrix3x2fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC       glProgramUniformMatrix2x4fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC       glProgramUniformMatrix4x2fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC       glProgramUniformMatrix3x4fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC       glProgramUniformMatrix4x3fv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC       glProgramUniformMatrix2x3dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC       glProgramUniformMatrix3x2dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC       glProgramUniformMatrix2x4dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC       glProgramUniformMatrix4x2dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC       glProgramUniformMatrix3x4dv = nullptr;
PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC       glProgramUniformMatrix4x3dv = nullptr;
PFNGLVALIDATEPROGRAMPIPELINEPROC         glValidateProgramPipeline = nullptr;
PFNGLGETPROGRAMPIPELINEINFOLOGPROC       glGetProgramPipelineInfoLog = nullptr;
PFNGLVERTEXATTRIBL1DPROC                 glVertexAttribL1d = nullptr;
PFNGLVERTEXATTRIBL2DPROC                 glVertexAttribL2d = nullptr;
PFNGLVERTEXATTRIBL3DPROC                 glVertexAttribL3d = nullptr;
PFNGLVERTEXATTRIBL4DPROC                 glVertexAttribL4d = nullptr;
PFNGLVERTEXATTRIBL1DVPROC                glVertexAttribL1dv = nullptr;
PFNGLVERTEXATTRIBL2DVPROC                glVertexAttribL2dv = nullptr;
PFNGLVERTEXATTRIBL3DVPROC                glVertexAttribL3dv = nullptr;
PFNGLVERTEXATTRIBL4DVPROC                glVertexAttribL4dv = nullptr;
PFNGLVERTEXATTRIBLPOINTERPROC            glVertexAttribLPointer = nullptr;
PFNGLGETVERTEXATTRIBLDVPROC              glGetVertexAttribLdv = nullptr;
PFNGLVIEWPORTARRAYVPROC                  glViewportArrayv = nullptr;
PFNGLVIEWPORTINDEXEDFPROC                glViewportIndexedf = nullptr;
PFNGLVIEWPORTINDEXEDFVPROC               glViewportIndexedfv = nullptr;
PFNGLSCISSORARRAYVPROC                   glScissorArrayv = nullptr;
PFNGLSCISSORINDEXEDPROC                  glScissorIndexed = nullptr;
PFNGLSCISSORINDEXEDVPROC                 glScissorIndexedv = nullptr;
PFNGLDEPTHRANGEARRAYVPROC                glDepthRangeArrayv = nullptr;
PFNGLDEPTHRANGEINDEXEDPROC               glDepthRangeIndexed = nullptr;
PFNGLGETFLOATI_VPROC                     glGetFloati_v = nullptr;
PFNGLGETDOUBLEI_VPROC                    glGetDoublei_v = nullptr;

// GL_VERSION_4_2
PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC             glDrawArraysInstancedBaseInstance = nullptr;
PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC           glDrawElementsInstancedBaseInstance = nullptr;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC glDrawElementsInstancedBaseVertexBaseInstance = nullptr;
PFNGLGETINTERNALFORMATIVPROC                         glGetInternalformativ = nullptr;
PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC              glGetActiveAtomicCounterBufferiv = nullptr;
PFNGLBINDIMAGETEXTUREPROC                            glBindImageTexture = nullptr;
PFNGLMEMORYBARRIERPROC                               glMemoryBarrier = nullptr;
PFNGLTEXSTORAGE1DPROC                                glTexStorage1D = nullptr;
PFNGLTEXSTORAGE2DPROC                                glTexStorage2D = nullptr;
PFNGLTEXSTORAGE3DPROC                                glTexStorage3D = nullptr;
PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC              glDrawTransformFeedbackInstanced = nullptr;
PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC        glDrawTransformFeedbackStreamInstanced = nullptr;

// GL_VERSION_4_3
PFNGLCLEARBUFFERDATAPROC                             glClearBufferData = nullptr;
PFNGLCLEARBUFFERSUBDATAPROC                          glClearBufferSubData = nullptr;
PFNGLDISPATCHCOMPUTEPROC                             glDispatchCompute = nullptr;
PFNGLDISPATCHCOMPUTEINDIRECTPROC                     glDispatchComputeIndirect = nullptr;
PFNGLCOPYIMAGESUBDATAPROC                            glCopyImageSubData = nullptr;
PFNGLFRAMEBUFFERPARAMETERIPROC                       glFramebufferParameteri = nullptr;
PFNGLGETFRAMEBUFFERPARAMETERIVPROC                   glGetFramebufferParameteriv = nullptr;
PFNGLGETINTERNALFORMATI64VPROC                       glGetInternalformati64v = nullptr;
PFNGLINVALIDATETEXSUBIMAGEPROC                       glInvalidateTexSubImage = nullptr;
PFNGLINVALIDATETEXIMAGEPROC                          glInvalidateTexImage = nullptr;
PFNGLINVALIDATEBUFFERSUBDATAPROC                     glInvalidateBufferSubData = nullptr;
PFNGLINVALIDATEBUFFERDATAPROC                        glInvalidateBufferData = nullptr;
PFNGLINVALIDATEFRAMEBUFFERPROC                       glInvalidateFramebuffer = nullptr;
PFNGLINVALIDATESUBFRAMEBUFFERPROC                    glInvalidateSubFramebuffer = nullptr;
PFNGLMULTIDRAWARRAYSINDIRECTPROC                     glMultiDrawArraysIndirect = nullptr;
PFNGLMULTIDRAWELEMENTSINDIRECTPROC                   glMultiDrawElementsIndirect = nullptr;
PFNGLGETPROGRAMINTERFACEIVPROC                       glGetProgramInterfaceiv = nullptr;
PFNGLGETPROGRAMRESOURCEINDEXPROC                     glGetProgramResourceIndex = nullptr;
PFNGLGETPROGRAMRESOURCENAMEPROC                      glGetProgramResourceName = nullptr;
PFNGLGETPROGRAMRESOURCEIVPROC                        glGetProgramResourceiv = nullptr;
PFNGLGETPROGRAMRESOURCELOCATIONPROC                  glGetProgramResourceLocation = nullptr;
PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC             glGetProgramResourceLocationIndex = nullptr;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC                   glShaderStorageBlockBinding = nullptr;
PFNGLTEXBUFFERRANGEPROC                              glTexBufferRange = nullptr;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC                     glTexStorage2DMultisample = nullptr;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC                     glTexStorage3DMultisample = nullptr;
PFNGLTEXTUREVIEWPROC                                 glTextureView = nullptr;
PFNGLBINDVERTEXBUFFERPROC                            glBindVertexBuffer = nullptr;
PFNGLVERTEXATTRIBFORMATPROC                          glVertexAttribFormat = nullptr;
PFNGLVERTEXATTRIBIFORMATPROC                         glVertexAttribIFormat = nullptr;
PFNGLVERTEXATTRIBLFORMATPROC                         glVertexAttribLFormat = nullptr;
PFNGLVERTEXATTRIBBINDINGPROC                         glVertexAttribBinding = nullptr;
PFNGLVERTEXBINDINGDIVISORPROC                        glVertexBindingDivisor = nullptr;
PFNGLDEBUGMESSAGECONTROLPROC                         glDebugMessageControl = nullptr;
PFNGLDEBUGMESSAGEINSERTPROC                          glDebugMessageInsert = nullptr;
PFNGLDEBUGMESSAGECALLBACKPROC                        glDebugMessageCallback = nullptr;
PFNGLGETDEBUGMESSAGELOGPROC                          glGetDebugMessageLog = nullptr;
PFNGLPUSHDEBUGGROUPPROC                              glPushDebugGroup = nullptr;
PFNGLPOPDEBUGGROUPPROC                               glPopDebugGroup = nullptr;
PFNGLOBJECTLABELPROC                                 glObjectLabel = nullptr;
PFNGLGETOBJECTLABELPROC                              glGetObjectLabel = nullptr;
PFNGLOBJECTPTRLABELPROC                              glObjectPtrLabel = nullptr;
PFNGLGETOBJECTPTRLABELPROC                           glGetObjectPtrLabel = nullptr;

// GL_VERSION_4_4
PFNGLBUFFERSTORAGEPROC       glBufferStorage = nullptr;
PFNGLCLEARTEXIMAGEPROC       glClearTexImage = nullptr;
PFNGLCLEARTEXSUBIMAGEPROC    glClearTexSubImage = nullptr;
PFNGLBINDBUFFERSBASEPROC     glBindBuffersBase = nullptr;
PFNGLBINDBUFFERSRANGEPROC    glBindBuffersRange = nullptr;
PFNGLBINDTEXTURESPROC        glBindTextures = nullptr;
PFNGLBINDSAMPLERSPROC        glBindSamplers = nullptr;
PFNGLBINDIMAGETEXTURESPROC   glBindImageTextures = nullptr;
PFNGLBINDVERTEXBUFFERSPROC   glBindVertexBuffers = nullptr;

// GL_VERSION_4_5
PFNGLCLIPCONTROLPROC                                     glClipControl = nullptr;
PFNGLCREATETRANSFORMFEEDBACKSPROC                        glCreateTransformFeedbacks = nullptr;
PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC                     glTransformFeedbackBufferBase = nullptr;
PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC                    glTransformFeedbackBufferRange = nullptr;
PFNGLGETTRANSFORMFEEDBACKIVPROC                          glGetTransformFeedbackiv = nullptr;
PFNGLGETTRANSFORMFEEDBACKI_VPROC                         glGetTransformFeedbacki_v = nullptr;
PFNGLGETTRANSFORMFEEDBACKI64_VPROC                       glGetTransformFeedbacki64_v = nullptr;
PFNGLCREATEBUFFERSPROC                                   glCreateBuffers = nullptr;
PFNGLNAMEDBUFFERSTORAGEPROC                              glNamedBufferStorage = nullptr;
PFNGLNAMEDBUFFERDATAPROC                                 glNamedBufferData = nullptr;
PFNGLNAMEDBUFFERSUBDATAPROC                              glNamedBufferSubData = nullptr;
PFNGLCOPYNAMEDBUFFERSUBDATAPROC                          glCopyNamedBufferSubData = nullptr;
PFNGLCLEARNAMEDBUFFERDATAPROC                            glClearNamedBufferData = nullptr;
PFNGLCLEARNAMEDBUFFERSUBDATAPROC                         glClearNamedBufferSubData = nullptr;
PFNGLMAPNAMEDBUFFERPROC                                  glMapNamedBuffer = nullptr;
PFNGLMAPNAMEDBUFFERRANGEPROC                             glMapNamedBufferRange = nullptr;
PFNGLUNMAPNAMEDBUFFERPROC                                glUnmapNamedBuffer = nullptr;
PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC                     glFlushMappedNamedBufferRange = nullptr;
PFNGLGETNAMEDBUFFERPARAMETERIVPROC                       glGetNamedBufferParameteriv = nullptr;
PFNGLGETNAMEDBUFFERPARAMETERI64VPROC                     glGetNamedBufferParameteri64v = nullptr;
PFNGLGETNAMEDBUFFERPOINTERVPROC                          glGetNamedBufferPointerv = nullptr;
PFNGLGETNAMEDBUFFERSUBDATAPROC                           glGetNamedBufferSubData = nullptr;
PFNGLCREATEFRAMEBUFFERSPROC                              glCreateFramebuffers = nullptr;
PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC                    glNamedFramebufferRenderbuffer = nullptr;
PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC                      glNamedFramebufferParameteri = nullptr;
PFNGLNAMEDFRAMEBUFFERTEXTUREPROC                         glNamedFramebufferTexture = nullptr;
PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC                    glNamedFramebufferTextureLayer = nullptr;
PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC                      glNamedFramebufferDrawBuffer = nullptr;
PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC                     glNamedFramebufferDrawBuffers = nullptr;
PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC                      glNamedFramebufferReadBuffer = nullptr;
PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC                  glInvalidateNamedFramebufferData = nullptr;
PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC               glInvalidateNamedFramebufferSubData = nullptr;
PFNGLCLEARNAMEDFRAMEBUFFERIVPROC                         glClearNamedFramebufferiv = nullptr;
PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC                        glClearNamedFramebufferuiv = nullptr;
PFNGLCLEARNAMEDFRAMEBUFFERFVPROC                         glClearNamedFramebufferfv = nullptr;
PFNGLCLEARNAMEDFRAMEBUFFERFIPROC                         glClearNamedFramebufferfi = nullptr;
PFNGLBLITNAMEDFRAMEBUFFERPROC                            glBlitNamedFramebuffer = nullptr;
PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC                     glCheckNamedFramebufferStatus = nullptr;
PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC                  glGetNamedFramebufferParameteriv = nullptr;
PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC        glGetNamedFramebufferAttachmentParameteriv = nullptr;
PFNGLCREATERENDERBUFFERSPROC                             glCreateRenderbuffers = nullptr;
PFNGLNAMEDRENDERBUFFERSTORAGEPROC                        glNamedRenderbufferStorage = nullptr;
PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC             glNamedRenderbufferStorageMultisample = nullptr;
PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC                 glGetNamedRenderbufferParameteriv = nullptr;
PFNGLCREATETEXTURESPROC                                  glCreateTextures = nullptr;
PFNGLTEXTUREBUFFERPROC                                   glTextureBuffer = nullptr;
PFNGLTEXTUREBUFFERRANGEPROC                              glTextureBufferRange = nullptr;
PFNGLTEXTURESTORAGE1DPROC                                glTextureStorage1D = nullptr;
PFNGLTEXTURESTORAGE2DPROC                                glTextureStorage2D = nullptr;
PFNGLTEXTURESTORAGE3DPROC                                glTextureStorage3D = nullptr;
PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC                     glTextureStorage2DMultisample = nullptr;
PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC                     glTextureStorage3DMultisample = nullptr;
PFNGLTEXTURESUBIMAGE1DPROC                               glTextureSubImage1D = nullptr;
PFNGLTEXTURESUBIMAGE2DPROC                               glTextureSubImage2D = nullptr;
PFNGLTEXTURESUBIMAGE3DPROC                               glTextureSubImage3D = nullptr;
PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC                     glCompressedTextureSubImage1D = nullptr;
PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC                     glCompressedTextureSubImage2D = nullptr;
PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC                     glCompressedTextureSubImage3D = nullptr;
PFNGLCOPYTEXTURESUBIMAGE1DPROC                           glCopyTextureSubImage1D = nullptr;
PFNGLCOPYTEXTURESUBIMAGE2DPROC                           glCopyTextureSubImage2D = nullptr;
PFNGLCOPYTEXTURESUBIMAGE3DPROC                           glCopyTextureSubImage3D = nullptr;
PFNGLTEXTUREPARAMETERFPROC                               glTextureParameterf = nullptr;
PFNGLTEXTUREPARAMETERFVPROC                              glTextureParameterfv = nullptr;
PFNGLTEXTUREPARAMETERIPROC                               glTextureParameteri = nullptr;
PFNGLTEXTUREPARAMETERIIVPROC                             glTextureParameterIiv = nullptr;
PFNGLTEXTUREPARAMETERIUIVPROC                            glTextureParameterIuiv = nullptr;
PFNGLTEXTUREPARAMETERIVPROC                              glTextureParameteriv = nullptr;
PFNGLGENERATETEXTUREMIPMAPPROC                           glGenerateTextureMipmap = nullptr;
PFNGLBINDTEXTUREUNITPROC                                 glBindTextureUnit = nullptr;
PFNGLGETTEXTUREIMAGEPROC                                 glGetTextureImage = nullptr;
PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC                       glGetCompressedTextureImage = nullptr;
PFNGLGETTEXTURELEVELPARAMETERFVPROC                      glGetTextureLevelParameterfv = nullptr;
PFNGLGETTEXTURELEVELPARAMETERIVPROC                      glGetTextureLevelParameteriv = nullptr;
PFNGLGETTEXTUREPARAMETERFVPROC                           glGetTextureParameterfv = nullptr;
PFNGLGETTEXTUREPARAMETERIIVPROC                          glGetTextureParameterIiv = nullptr;
PFNGLGETTEXTUREPARAMETERIUIVPROC                         glGetTextureParameterIuiv = nullptr;
PFNGLGETTEXTUREPARAMETERIVPROC                           glGetTextureParameteriv = nullptr;
PFNGLCREATEVERTEXARRAYSPROC                              glCreateVertexArrays = nullptr;
PFNGLDISABLEVERTEXARRAYATTRIBPROC                        glDisableVertexArrayAttrib = nullptr;
PFNGLENABLEVERTEXARRAYATTRIBPROC                         glEnableVertexArrayAttrib = nullptr;
PFNGLVERTEXARRAYELEMENTBUFFERPROC                        glVertexArrayElementBuffer = nullptr;
PFNGLVERTEXARRAYVERTEXBUFFERPROC                         glVertexArrayVertexBuffer = nullptr;
PFNGLVERTEXARRAYVERTEXBUFFERSPROC                        glVertexArrayVertexBuffers = nullptr;
PFNGLVERTEXARRAYATTRIBBINDINGPROC                        glVertexArrayAttribBinding = nullptr;
PFNGLVERTEXARRAYATTRIBFORMATPROC                         glVertexArrayAttribFormat = nullptr;
PFNGLVERTEXARRAYATTRIBIFORMATPROC                        glVertexArrayAttribIFormat = nullptr;
PFNGLVERTEXARRAYATTRIBLFORMATPROC                        glVertexArrayAttribLFormat = nullptr;
PFNGLVERTEXARRAYBINDINGDIVISORPROC                       glVertexArrayBindingDivisor = nullptr;
PFNGLGETVERTEXARRAYIVPROC                                glGetVertexArrayiv = nullptr;
PFNGLGETVERTEXARRAYINDEXEDIVPROC                         glGetVertexArrayIndexediv = nullptr;
PFNGLGETVERTEXARRAYINDEXED64IVPROC                       glGetVertexArrayIndexed64iv = nullptr;
PFNGLCREATESAMPLERSPROC                                  glCreateSamplers = nullptr;
PFNGLCREATEPROGRAMPIPELINESPROC                          glCreateProgramPipelines = nullptr;
PFNGLCREATEQUERIESPROC                                   glCreateQueries = nullptr;
PFNGLGETQUERYBUFFEROBJECTI64VPROC                        glGetQueryBufferObjecti64v = nullptr;
PFNGLGETQUERYBUFFEROBJECTIVPROC                          glGetQueryBufferObjectiv = nullptr;
PFNGLGETQUERYBUFFEROBJECTUI64VPROC                       glGetQueryBufferObjectui64v = nullptr;
PFNGLGETQUERYBUFFEROBJECTUIVPROC                         glGetQueryBufferObjectuiv = nullptr;
PFNGLMEMORYBARRIERBYREGIONPROC                           glMemoryBarrierByRegion = nullptr;
PFNGLGETTEXTURESUBIMAGEPROC                              glGetTextureSubImage = nullptr;
PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC                    glGetCompressedTextureSubImage = nullptr;
PFNGLGETGRAPHICSRESETSTATUSPROC                          glGetGraphicsResetStatus = nullptr;
PFNGLGETNCOMPRESSEDTEXIMAGEPROC                          glGetnCompressedTexImage = nullptr;
PFNGLGETNTEXIMAGEPROC                                    glGetnTexImage = nullptr;
PFNGLGETNUNIFORMDVPROC                                   glGetnUniformdv = nullptr;
PFNGLGETNUNIFORMFVPROC                                   glGetnUniformfv = nullptr;
PFNGLGETNUNIFORMIVPROC                                   glGetnUniformiv = nullptr;
PFNGLGETNUNIFORMUIVPROC                                  glGetnUniformuiv = nullptr;
PFNGLREADNPIXELSPROC                                     glReadnPixels = nullptr;
PFNGLGETNMAPDVPROC                                       glGetnMapdv = nullptr;
PFNGLGETNMAPFVPROC                                       glGetnMapfv = nullptr;
PFNGLGETNMAPIVPROC                                       glGetnMapiv = nullptr;
PFNGLGETNPIXELMAPFVPROC                                  glGetnPixelMapfv = nullptr;
PFNGLGETNPIXELMAPUIVPROC                                 glGetnPixelMapuiv = nullptr;
PFNGLGETNPIXELMAPUSVPROC                                 glGetnPixelMapusv = nullptr;
PFNGLGETNPOLYGONSTIPPLEPROC                              glGetnPolygonStipple = nullptr;
PFNGLGETNCOLORTABLEPROC                                  glGetnColorTable = nullptr;
PFNGLGETNCONVOLUTIONFILTERPROC                           glGetnConvolutionFilter = nullptr;
PFNGLGETNSEPARABLEFILTERPROC                             glGetnSeparableFilter = nullptr;
PFNGLGETNHISTOGRAMPROC                                   glGetnHistogram = nullptr;
PFNGLGETNMINMAXPROC                                      glGetnMinmax = nullptr;
PFNGLTEXTUREBARRIERPROC                                  glTextureBarrier = nullptr;

// GL_VERSION_4_6
PFNGLSPECIALIZESHADERPROC                glSpecializeShader = nullptr;
PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC    glMultiDrawArraysIndirectCount = nullptr;
PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC  glMultiDrawElementsIndirectCount = nullptr;
PFNGLPOLYGONOFFSETCLAMPPROC              glPolygonOffsetClamp = nullptr;

#endif

LLGLManager gGLManager;

LLGLManager::LLGLManager() :
	mInited(false),
	mIsDisabled(false),
	mMaxSamples(0),
	mNumTextureImageUnits(1),
	mMaxSampleMaskWords(0),
	mMaxColorTextureSamples(0),
	mMaxDepthTextureSamples(0),
	mMaxIntegerSamples(0),
	mIsAMD(false),
	mIsNVIDIA(false),
	mIsIntel(false),
#if LL_DARWIN
	mIsMobileGF(false),
#endif
	mHasRequirements(true),
	mDriverVersionMajor(1),
	mDriverVersionMinor(0),
	mDriverVersionRelease(0),
	mGLVersion(1.0f),
	mGLSLVersionMajor(0),
	mGLSLVersionMinor(0),		
	mVRAM(0),
	mGLMaxVertexRange(0),
	mGLMaxIndexRange(0)
{
}

//---------------------------------------------------------------------
// Global initialization for GL
//---------------------------------------------------------------------
void LLGLManager::initWGL()
{
#if LL_WINDOWS && !LL_MESA_HEADLESS
	if (!glh_init_extensions("WGL_ARB_pixel_format"))
	{
		LL_WARNS("RenderInit") << "No ARB pixel format extensions" << LL_ENDL;
	}

	if (ExtensionExists("WGL_ARB_create_context",gGLHExts.mSysExts))
	{
		GLH_EXT_NAME(wglCreateContextAttribsARB) = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GLH_EXT_GET_PROC_ADDRESS("wglCreateContextAttribsARB");
	}
	else
	{
		LL_WARNS("RenderInit") << "No ARB create context extensions" << LL_ENDL;
	}

	// For retreiving information per AMD adapter, 
	// because we can't trust curently selected/default one when there are multiple
	mHasAMDAssociations = ExtensionExists("WGL_AMD_gpu_association", gGLHExts.mSysExts);
	if (mHasAMDAssociations)
	{
		GLH_EXT_NAME(wglGetGPUIDsAMD) = (PFNWGLGETGPUIDSAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetGPUIDsAMD");
		GLH_EXT_NAME(wglGetGPUInfoAMD) = (PFNWGLGETGPUINFOAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetGPUInfoAMD");
	}

	if (ExtensionExists("WGL_EXT_swap_control", gGLHExts.mSysExts))
	{
        GLH_EXT_NAME(wglSwapIntervalEXT) = (PFNWGLSWAPINTERVALEXTPROC)GLH_EXT_GET_PROC_ADDRESS("wglSwapIntervalEXT");
	}

	if( !glh_init_extensions("WGL_ARB_pbuffer") )
	{
		LL_WARNS("RenderInit") << "No ARB WGL PBuffer extensions" << LL_ENDL;
	}

	if( !glh_init_extensions("WGL_ARB_render_texture") )
	{
		LL_WARNS("RenderInit") << "No ARB WGL render texture extensions" << LL_ENDL;
	}
#endif
}

// return false if unable (or unwilling due to old drivers) to init GL
bool LLGLManager::initGL()
{
	if (mInited)
	{
		LL_ERRS("RenderInit") << "Calling init on LLGLManager after already initialized!" << LL_ENDL;
	}

#if 0 && LL_WINDOWS
	if (!glGetStringi)
	{
		glGetStringi = (PFNGLGETSTRINGIPROC) GLH_EXT_GET_PROC_ADDRESS("glGetStringi");
	}

	//reload extensions string (may have changed after using wglCreateContextAttrib)
	if (glGetStringi)
	{
		std::stringstream str;

		GLint count = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &count);
		for (GLint i = 0; i < count; ++i)
		{
			std::string ext = ll_safe_string((const char*) glGetStringi(GL_EXTENSIONS, i));
			str << ext << " ";
			LL_DEBUGS("GLExtensions") << ext << LL_ENDL;
		}
		
		{
			PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
			wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
			if(wglGetExtensionsStringARB)
			{
				str << (const char*) wglGetExtensionsStringARB(wglGetCurrentDC());
			}
		}

		free(gGLHExts.mSysExts);
		std::string extensions = str.str();
		gGLHExts.mSysExts = strdup(extensions.c_str());
	}
#endif
	
	// Extract video card strings and convert to upper case to
	// work around driver-to-driver variation in capitalization.
	mGLVendor = ll_safe_string((const char *)glGetString(GL_VENDOR));
	LLStringUtil::toUpper(mGLVendor);

	mGLRenderer = ll_safe_string((const char *)glGetString(GL_RENDERER));
	LLStringUtil::toUpper(mGLRenderer);

	parse_gl_version( &mDriverVersionMajor, 
		&mDriverVersionMinor, 
		&mDriverVersionRelease, 
		&mDriverVersionVendorString,
		&mGLVersionString);

	mGLVersion = mDriverVersionMajor + mDriverVersionMinor * .1f;

	if (mGLVersion >= 2.f)
	{
		parse_glsl_version(mGLSLVersionMajor, mGLSLVersionMinor);

#if 0 && LL_DARWIN
		// TODO maybe switch to using a core profile for GL 3.2?
		// https://stackoverflow.com/a/19868861
		//never use GLSL greater than 1.20 on OSX
		if (mGLSLVersionMajor > 1 || mGLSLVersionMinor > 30)
		{
			mGLSLVersionMajor = 1;
			mGLSLVersionMinor = 30;
		}
#endif
	}

	if (mGLVersion >= 2.1f && LLImageGL::sCompressTextures)
	{ //use texture compression
		glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
	}
	else
	{ //GL version is < 3.0, always disable texture compression
		LLImageGL::sCompressTextures = false;
	}
	
	// Trailing space necessary to keep "nVidia Corpor_ati_on" cards
	// from being recognized as ATI.
    // NOTE: AMD has been pretty good about not breaking this check, do not rename without good reason
	if (mGLVendor.substr(0,4) == "ATI ")
	{
		mGLVendorShort = "AMD";
		// *TODO: Fix this?
		mIsAMD = true;
	}
	else if (mGLVendor.find("NVIDIA ") != std::string::npos)
	{
		mGLVendorShort = "NVIDIA";
		mIsNVIDIA = true;
	}
	else if (mGLVendor.find("INTEL") != std::string::npos
#if LL_LINUX
		 // The Mesa-based drivers put this in the Renderer string,
		 // not the Vendor string.
		 || mGLRenderer.find("INTEL") != std::string::npos
#endif //LL_LINUX
		 )
	{
		mGLVendorShort = "INTEL";
		mIsIntel = true;
	}
	else
	{
		mGLVendorShort = "MISC";
	}
	
	// This is called here because it depends on the setting of mIsGF2or4MX, and sets up mHasMultitexture.
	initExtensions();

	S32 old_vram = mVRAM;
	mVRAM = 0;

#if LL_WINDOWS
	if (mHasAMDAssociations)
	{
		GLuint gl_gpus_count = wglGetGPUIDsAMD(0, 0);
		if (gl_gpus_count > 0)
		{
			GLuint* ids = new GLuint[gl_gpus_count];
			wglGetGPUIDsAMD(gl_gpus_count, ids);

			GLuint mem_mb = 0;
			for (U32 i = 0; i < gl_gpus_count; i++)
			{
				wglGetGPUInfoAMD(ids[i],
					WGL_GPU_RAM_AMD,
					GL_UNSIGNED_INT,
					sizeof(GLuint),
					&mem_mb);
				if (mVRAM < mem_mb)
				{
					// basically pick the best AMD and trust driver/OS to know to switch
					mVRAM = mem_mb;
				}
			}
		}
		if (mVRAM != 0)
		{
			LL_WARNS("RenderInit") << "VRAM Detected (AMDAssociations):" << mVRAM << LL_ENDL;
		}
	}
#endif

#if LL_WINDOWS
	if (mVRAM < 256)
	{
		// Something likely went wrong using the above extensions
		// try WMI first and fall back to old method (from dxdiag) if all else fails
		// Function will check all GPUs WMI knows of and will pick up the one with most
		// memory. We need to check all GPUs because system can switch active GPU to
		// weaker one, to preserve power when not under load.
		S32 mem = LLDXHardware::getMBVideoMemoryViaWMI();
		if (mem != 0)
		{
			mVRAM = mem;
			LL_WARNS("RenderInit") << "VRAM Detected (WMI):" << mVRAM<< LL_ENDL;
		}
	}
#endif

	if (mVRAM < 256 && old_vram > 0)
	{
		// fall back to old method
		// Note: on Windows value will be from LLDXHardware.
		// Either received via dxdiag or via WMI by id from dxdiag.
		mVRAM = old_vram;
	}

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mNumTextureImageUnits);
	glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &mMaxColorTextureSamples);
	glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &mMaxDepthTextureSamples);
	glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &mMaxIntegerSamples);
	glGetIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &mMaxSampleMaskWords);
    glGetIntegerv(GL_MAX_SAMPLES, &mMaxSamples);

    if (mGLVersion >= 4.59f)
    {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &mMaxAnisotropy);
    }

	initGLStates();

	return true;
}

void LLGLManager::getGLInfo(LLSD& info)
{
	if (gHeadlessClient)
	{
		info["GLInfo"]["GLVendor"] = HEADLESS_VENDOR_STRING;
		info["GLInfo"]["GLRenderer"] = HEADLESS_RENDERER_STRING;
		info["GLInfo"]["GLVersion"] = HEADLESS_VERSION_STRING;
		return;
	}
	else
	{
		info["GLInfo"]["GLVendor"] = ll_safe_string((const char *)glGetString(GL_VENDOR));
		info["GLInfo"]["GLRenderer"] = ll_safe_string((const char *)glGetString(GL_RENDERER));
		info["GLInfo"]["GLVersion"] = ll_safe_string((const char *)glGetString(GL_VERSION));
	}

#if !LL_MESA_HEADLESS
	std::string all_exts = ll_safe_string((const char *)gGLHExts.mSysExts);
	boost::char_separator<char> sep(" ");
	boost::tokenizer<boost::char_separator<char> > tok(all_exts, sep);
	for(boost::tokenizer<boost::char_separator<char> >::iterator i = tok.begin(); i != tok.end(); ++i)
	{
		info["GLInfo"]["GLExtensions"].append(*i);
	}
#endif
}

std::string LLGLManager::getGLInfoString()
{
	std::string info_str;

	if (gHeadlessClient)
	{
		info_str += std::string("GL_VENDOR      ") + HEADLESS_VENDOR_STRING + std::string("\n");
		info_str += std::string("GL_RENDERER    ") + HEADLESS_RENDERER_STRING + std::string("\n");
		info_str += std::string("GL_VERSION     ") + HEADLESS_VERSION_STRING + std::string("\n");
	}
	else
	{
		info_str += std::string("GL_VENDOR      ") + ll_safe_string((const char *)glGetString(GL_VENDOR)) + std::string("\n");
		info_str += std::string("GL_RENDERER    ") + ll_safe_string((const char *)glGetString(GL_RENDERER)) + std::string("\n");
		info_str += std::string("GL_VERSION     ") + ll_safe_string((const char *)glGetString(GL_VERSION)) + std::string("\n");
	}

#if !LL_MESA_HEADLESS 
	std::string all_exts= ll_safe_string(((const char *)gGLHExts.mSysExts));
	LLStringUtil::replaceChar(all_exts, ' ', '\n');
	info_str += std::string("GL_EXTENSIONS:\n") + all_exts + std::string("\n");
#endif
	
	return info_str;
}

void LLGLManager::printGLInfoString()
{
	if (gHeadlessClient)
	{
		LL_INFOS("RenderInit") << "GL_VENDOR:     " << HEADLESS_VENDOR_STRING << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_RENDERER:   " << HEADLESS_RENDERER_STRING << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_VERSION:    " << HEADLESS_VERSION_STRING << LL_ENDL;
	}
	else
	{
		LL_INFOS("RenderInit") << "GL_VENDOR:     " << ll_safe_string((const char *)glGetString(GL_VENDOR)) << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_RENDERER:   " << ll_safe_string((const char *)glGetString(GL_RENDERER)) << LL_ENDL;
		LL_INFOS("RenderInit") << "GL_VERSION:    " << ll_safe_string((const char *)glGetString(GL_VERSION)) << LL_ENDL;
	}

#if !LL_MESA_HEADLESS
	std::string all_exts= ll_safe_string(((const char *)gGLHExts.mSysExts));
	LLStringUtil::replaceChar(all_exts, ' ', '\n');
	LL_DEBUGS("RenderInit") << "GL_EXTENSIONS:\n" << all_exts << LL_ENDL;
#endif
}

std::string LLGLManager::getRawGLString()
{
	std::string gl_string;
	if (gHeadlessClient)
	{
		gl_string = HEADLESS_VENDOR_STRING + " " + HEADLESS_RENDERER_STRING;
	}
	else
	{
		gl_string = ll_safe_string((char*)glGetString(GL_VENDOR)) + " " + ll_safe_string((char*)glGetString(GL_RENDERER));
	}
	return gl_string;
}

void LLGLManager::asLLSD(LLSD& info)
{
	// Currently these are duplicates of fields in "system".
	info["gpu_vendor"] = mGLVendorShort;
	info["gpu_version"] = mDriverVersionVendorString;
	info["opengl_version"] = mGLVersionString;

	info["vram"] = mVRAM;

	// OpenGL limits
	info["max_samples"] = mMaxSamples;
	info["num_texture_image_units"] =  mNumTextureImageUnits;
	info["max_sample_mask_words"] = mMaxSampleMaskWords;
	info["max_color_texture_samples"] = mMaxColorTextureSamples;
	info["max_depth_texture_samples"] = mMaxDepthTextureSamples;
	info["max_integer_samples"] = mMaxIntegerSamples;
    info["max_vertex_range"] = mGLMaxVertexRange;
    info["max_index_range"] = mGLMaxIndexRange;
    info["max_texture_size"] = mGLMaxTextureSize;

	// Which vendor
	info["is_ati"] = mIsAMD;  // note, do not rename is_ati to is_amd without coordinating with DW
	info["is_nvidia"] = mIsNVIDIA;
	info["is_intel"] = mIsIntel;

	info["gl_renderer"] = mGLRenderer;
}

void LLGLManager::shutdownGL()
{
	if (mInited)
	{
		glFinish();
		stop_glerror();
		mInited = false;
	}
}

// these are used to turn software blending on. They appear in the Debug/Avatar menu
// presence of vertex skinning/blending or vertex programs will set these to false by default.

void LLGLManager::initExtensions()
{
#if LL_DARWIN
    GLint num_extensions = 0;
    std::string all_extensions{""};
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for(GLint i = 0; i < num_extensions; ++i) {
        char const * extension = (char const *)glGetStringi(GL_EXTENSIONS, i);
        all_extensions += extension;
        all_extensions += ' ';
    }
    if (num_extensions)
    {
        all_extensions += "GL_ARB_multitexture GL_ARB_texture_cube_map GL_ARB_texture_compression "; // These are in 3.2 core, but not listed by OSX
        gGLHExts.mSysExts = strdup(all_extensions.data());
    }
#endif

    // NOTE: version checks against mGLVersion should bias down by 0.01 because of F32 errors
    
    // OpenGL 4.x capabilities
    mHasCubeMapArray = mGLVersion >= 3.99f; 
    mHasTransformFeedback = mGLVersion >= 3.99f;
    mHasDebugOutput = mGLVersion >= 4.29f;

    // Misc
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, (GLint*) &mGLMaxVertexRange);
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, (GLint*) &mGLMaxIndexRange);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*) &mGLMaxTextureSize);

    mInited = true;

#if (LL_WINDOWS || LL_LINUX) && !LL_MESA_HEADLESS
	LL_DEBUGS("RenderInit") << "GL Probe: Getting symbols" << LL_ENDL;
	
#if LL_WINDOWS
    // WGL_AMD_gpu_association
    wglGetGPUIDsAMD = (PFNWGLGETGPUIDSAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetGPUIDsAMD");
    wglGetGPUInfoAMD = (PFNWGLGETGPUINFOAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetGPUInfoAMD");
    wglGetContextGPUIDAMD = (PFNWGLGETCONTEXTGPUIDAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetContextGPUIDAMD");
    wglCreateAssociatedContextAMD = (PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglCreateAssociatedContextAMD");
    wglCreateAssociatedContextAttribsAMD = (PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglCreateAssociatedContextAttribsAMD");
    wglDeleteAssociatedContextAMD = (PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglDeleteAssociatedContextAMD");
    wglMakeAssociatedContextCurrentAMD = (PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglMakeAssociatedContextCurrentAMD");
    wglGetCurrentAssociatedContextAMD = (PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetCurrentAssociatedContextAMD");
    wglBlitContextFramebufferAMD = (PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC)GLH_EXT_GET_PROC_ADDRESS("wglBlitContextFramebufferAMD");

    // WGL_EXT_swap_control
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)GLH_EXT_GET_PROC_ADDRESS("wglSwapIntervalEXT");
    wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)GLH_EXT_GET_PROC_ADDRESS("wglGetSwapIntervalEXT");

    // WGL_ARB_create_context
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GLH_EXT_GET_PROC_ADDRESS("wglCreateContextAttribsARB");
#endif


    // Load entire OpenGL API through GetProcAddress, leaving sections beyond mGLVersion unloaded

    // GL_VERSION_1_2
    if (mGLVersion < 1.19f)
    {
        return;
    }
    glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawRangeElements");
    glTexImage3D = (PFNGLTEXIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glTexImage3D");
    glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glTexSubImage3D");
    glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glCopyTexSubImage3D");


    // GL_VERSION_1_3
    if (mGLVersion < 1.29f)
    {
        return;
    }
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)GLH_EXT_GET_PROC_ADDRESS("glActiveTexture");
    glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glSampleCoverage");
    glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTexImage3D");
    glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTexImage2D");
    glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTexImage1D");
    glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTexSubImage3D");
    glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTexSubImage2D");
    glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTexSubImage1D");
    glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetCompressedTexImage");
    glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)GLH_EXT_GET_PROC_ADDRESS("glClientActiveTexture");
    glMultiTexCoord1d = (PFNGLMULTITEXCOORD1DPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1d");
    glMultiTexCoord1dv = (PFNGLMULTITEXCOORD1DVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1dv");
    glMultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1f");
    glMultiTexCoord1fv = (PFNGLMULTITEXCOORD1FVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1fv");
    glMultiTexCoord1i = (PFNGLMULTITEXCOORD1IPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1i");
    glMultiTexCoord1iv = (PFNGLMULTITEXCOORD1IVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1iv");
    glMultiTexCoord1s = (PFNGLMULTITEXCOORD1SPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1s");
    glMultiTexCoord1sv = (PFNGLMULTITEXCOORD1SVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord1sv");
    glMultiTexCoord2d = (PFNGLMULTITEXCOORD2DPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2d");
    glMultiTexCoord2dv = (PFNGLMULTITEXCOORD2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2dv");
    glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2f");
    glMultiTexCoord2fv = (PFNGLMULTITEXCOORD2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2fv");
    glMultiTexCoord2i = (PFNGLMULTITEXCOORD2IPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2i");
    glMultiTexCoord2iv = (PFNGLMULTITEXCOORD2IVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2iv");
    glMultiTexCoord2s = (PFNGLMULTITEXCOORD2SPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2s");
    glMultiTexCoord2sv = (PFNGLMULTITEXCOORD2SVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord2sv");
    glMultiTexCoord3d = (PFNGLMULTITEXCOORD3DPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3d");
    glMultiTexCoord3dv = (PFNGLMULTITEXCOORD3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3dv");
    glMultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3f");
    glMultiTexCoord3fv = (PFNGLMULTITEXCOORD3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3fv");
    glMultiTexCoord3i = (PFNGLMULTITEXCOORD3IPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3i");
    glMultiTexCoord3iv = (PFNGLMULTITEXCOORD3IVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3iv");
    glMultiTexCoord3s = (PFNGLMULTITEXCOORD3SPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3s");
    glMultiTexCoord3sv = (PFNGLMULTITEXCOORD3SVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord3sv");
    glMultiTexCoord4d = (PFNGLMULTITEXCOORD4DPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4d");
    glMultiTexCoord4dv = (PFNGLMULTITEXCOORD4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4dv");
    glMultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4f");
    glMultiTexCoord4fv = (PFNGLMULTITEXCOORD4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4fv");
    glMultiTexCoord4i = (PFNGLMULTITEXCOORD4IPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4i");
    glMultiTexCoord4iv = (PFNGLMULTITEXCOORD4IVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4iv");
    glMultiTexCoord4s = (PFNGLMULTITEXCOORD4SPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4s");
    glMultiTexCoord4sv = (PFNGLMULTITEXCOORD4SVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoord4sv");
    glLoadTransposeMatrixf = (PFNGLLOADTRANSPOSEMATRIXFPROC)GLH_EXT_GET_PROC_ADDRESS("glLoadTransposeMatrixf");
    glLoadTransposeMatrixd = (PFNGLLOADTRANSPOSEMATRIXDPROC)GLH_EXT_GET_PROC_ADDRESS("glLoadTransposeMatrixd");
    glMultTransposeMatrixf = (PFNGLMULTTRANSPOSEMATRIXFPROC)GLH_EXT_GET_PROC_ADDRESS("glMultTransposeMatrixf");
    glMultTransposeMatrixd = (PFNGLMULTTRANSPOSEMATRIXDPROC)GLH_EXT_GET_PROC_ADDRESS("glMultTransposeMatrixd");

    // GL_VERSION_1_4
    if (mGLVersion < 1.39f)
    {
        return;
    }
    glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)GLH_EXT_GET_PROC_ADDRESS("glBlendFuncSeparate");
    glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiDrawArrays");
    glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiDrawElements");
    glPointParameterf = (PFNGLPOINTPARAMETERFPROC)GLH_EXT_GET_PROC_ADDRESS("glPointParameterf");
    glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glPointParameterfv");
    glPointParameteri = (PFNGLPOINTPARAMETERIPROC)GLH_EXT_GET_PROC_ADDRESS("glPointParameteri");
    glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glPointParameteriv");
    glFogCoordf = (PFNGLFOGCOORDFPROC)GLH_EXT_GET_PROC_ADDRESS("glFogCoordf");
    glFogCoordfv = (PFNGLFOGCOORDFVPROC)GLH_EXT_GET_PROC_ADDRESS("glFogCoordfv");
    glFogCoordd = (PFNGLFOGCOORDDPROC)GLH_EXT_GET_PROC_ADDRESS("glFogCoordd");
    glFogCoorddv = (PFNGLFOGCOORDDVPROC)GLH_EXT_GET_PROC_ADDRESS("glFogCoorddv");
    glFogCoordPointer = (PFNGLFOGCOORDPOINTERPROC)GLH_EXT_GET_PROC_ADDRESS("glFogCoordPointer");
    glSecondaryColor3b = (PFNGLSECONDARYCOLOR3BPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3b");
    glSecondaryColor3bv = (PFNGLSECONDARYCOLOR3BVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3bv");
    glSecondaryColor3d = (PFNGLSECONDARYCOLOR3DPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3d");
    glSecondaryColor3dv = (PFNGLSECONDARYCOLOR3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3dv");
    glSecondaryColor3f = (PFNGLSECONDARYCOLOR3FPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3f");
    glSecondaryColor3fv = (PFNGLSECONDARYCOLOR3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3fv");
    glSecondaryColor3i = (PFNGLSECONDARYCOLOR3IPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3i");
    glSecondaryColor3iv = (PFNGLSECONDARYCOLOR3IVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3iv");
    glSecondaryColor3s = (PFNGLSECONDARYCOLOR3SPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3s");
    glSecondaryColor3sv = (PFNGLSECONDARYCOLOR3SVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3sv");
    glSecondaryColor3ub = (PFNGLSECONDARYCOLOR3UBPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3ub");
    glSecondaryColor3ubv = (PFNGLSECONDARYCOLOR3UBVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3ubv");
    glSecondaryColor3ui = (PFNGLSECONDARYCOLOR3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3ui");
    glSecondaryColor3uiv = (PFNGLSECONDARYCOLOR3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3uiv");
    glSecondaryColor3us = (PFNGLSECONDARYCOLOR3USPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3us");
    glSecondaryColor3usv = (PFNGLSECONDARYCOLOR3USVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColor3usv");
    glSecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTERPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColorPointer");
    glWindowPos2d = (PFNGLWINDOWPOS2DPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2d");
    glWindowPos2dv = (PFNGLWINDOWPOS2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2dv");
    glWindowPos2f = (PFNGLWINDOWPOS2FPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2f");
    glWindowPos2fv = (PFNGLWINDOWPOS2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2fv");
    glWindowPos2i = (PFNGLWINDOWPOS2IPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2i");
    glWindowPos2iv = (PFNGLWINDOWPOS2IVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2iv");
    glWindowPos2s = (PFNGLWINDOWPOS2SPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2s");
    glWindowPos2sv = (PFNGLWINDOWPOS2SVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos2sv");
    glWindowPos3d = (PFNGLWINDOWPOS3DPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3d");
    glWindowPos3dv = (PFNGLWINDOWPOS3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3dv");
    glWindowPos3f = (PFNGLWINDOWPOS3FPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3f");
    glWindowPos3fv = (PFNGLWINDOWPOS3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3fv");
    glWindowPos3i = (PFNGLWINDOWPOS3IPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3i");
    glWindowPos3iv = (PFNGLWINDOWPOS3IVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3iv");
    glWindowPos3s = (PFNGLWINDOWPOS3SPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3s");
    glWindowPos3sv = (PFNGLWINDOWPOS3SVPROC)GLH_EXT_GET_PROC_ADDRESS("glWindowPos3sv");

    // GL_VERSION_1_5
    if (mGLVersion < 1.49f)
    {
        return;
    }
    glGenQueries = (PFNGLGENQUERIESPROC)GLH_EXT_GET_PROC_ADDRESS("glGenQueries");
    glDeleteQueries = (PFNGLDELETEQUERIESPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteQueries");
    glIsQuery = (PFNGLISQUERYPROC)GLH_EXT_GET_PROC_ADDRESS("glIsQuery");
    glBeginQuery = (PFNGLBEGINQUERYPROC)GLH_EXT_GET_PROC_ADDRESS("glBeginQuery");
    glEndQuery = (PFNGLENDQUERYPROC)GLH_EXT_GET_PROC_ADDRESS("glEndQuery");
    glGetQueryiv = (PFNGLGETQUERYIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryiv");
    glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjectiv");
    glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjectuiv");
    glBindBuffer = (PFNGLBINDBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glBindBuffer");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteBuffers");
    glGenBuffers = (PFNGLGENBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glGenBuffers");
    glIsBuffer = (PFNGLISBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glIsBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glBufferData");
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glBufferSubData");
    glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBufferSubData");
    glMapBuffer = (PFNGLMAPBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glMapBuffer");
    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glUnmapBuffer");
    glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBufferParameteriv");
    glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBufferPointerv");

    // GL_VERSION_2_0
    if (mGLVersion < 1.9f)
    {
        return;
    }
    glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)GLH_EXT_GET_PROC_ADDRESS("glBlendEquationSeparate");
    glDrawBuffers = (PFNGLDRAWBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawBuffers");
    glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)GLH_EXT_GET_PROC_ADDRESS("glStencilOpSeparate");
    glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)GLH_EXT_GET_PROC_ADDRESS("glStencilFuncSeparate");
    glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)GLH_EXT_GET_PROC_ADDRESS("glStencilMaskSeparate");
    glAttachShader = (PFNGLATTACHSHADERPROC)GLH_EXT_GET_PROC_ADDRESS("glAttachShader");
    glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)GLH_EXT_GET_PROC_ADDRESS("glBindAttribLocation");
    glCompileShader = (PFNGLCOMPILESHADERPROC)GLH_EXT_GET_PROC_ADDRESS("glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateProgram");
    glCreateShader = (PFNGLCREATESHADERPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateShader");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteProgram");
    glDeleteShader = (PFNGLDELETESHADERPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteShader");
    glDetachShader = (PFNGLDETACHSHADERPROC)GLH_EXT_GET_PROC_ADDRESS("glDetachShader");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)GLH_EXT_GET_PROC_ADDRESS("glDisableVertexAttribArray");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)GLH_EXT_GET_PROC_ADDRESS("glEnableVertexAttribArray");
    glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveAttrib");
    glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveUniform");
    glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)GLH_EXT_GET_PROC_ADDRESS("glGetAttachedShaders");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)GLH_EXT_GET_PROC_ADDRESS("glGetAttribLocation");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramInfoLog");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GLH_EXT_GET_PROC_ADDRESS("glGetShaderInfoLog");
    glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetShaderSource");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformLocation");
    glGetUniformfv = (PFNGLGETUNIFORMFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformfv");
    glGetUniformiv = (PFNGLGETUNIFORMIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformiv");
    glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribdv");
    glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribfv");
    glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribiv");
    glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribPointerv");
    glIsProgram = (PFNGLISPROGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glIsProgram");
    glIsShader = (PFNGLISSHADERPROC)GLH_EXT_GET_PROC_ADDRESS("glIsShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glLinkProgram");
    glShaderSource = (PFNGLSHADERSOURCEPROC)GLH_EXT_GET_PROC_ADDRESS("glShaderSource");
    glUseProgram = (PFNGLUSEPROGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glUseProgram");
    glUniform1f = (PFNGLUNIFORM1FPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1f");
    glUniform2f = (PFNGLUNIFORM2FPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2f");
    glUniform3f = (PFNGLUNIFORM3FPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4f");
    glUniform1i = (PFNGLUNIFORM1IPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1i");
    glUniform2i = (PFNGLUNIFORM2IPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2i");
    glUniform3i = (PFNGLUNIFORM3IPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3i");
    glUniform4i = (PFNGLUNIFORM4IPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4i");
    glUniform1fv = (PFNGLUNIFORM1FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1fv");
    glUniform2fv = (PFNGLUNIFORM2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2fv");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3fv");
    glUniform4fv = (PFNGLUNIFORM4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4fv");
    glUniform1iv = (PFNGLUNIFORM1IVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1iv");
    glUniform2iv = (PFNGLUNIFORM2IVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2iv");
    glUniform3iv = (PFNGLUNIFORM3IVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3iv");
    glUniform4iv = (PFNGLUNIFORM4IVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4iv");
    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix2fv");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3fv");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix4fv");
    glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glValidateProgram");
    glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1d");
    glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1dv");
    glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1f");
    glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1fv");
    glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1s");
    glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib1sv");
    glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2d");
    glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2dv");
    glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2f");
    glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2fv");
    glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2s");
    glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib2sv");
    glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3d");
    glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3dv");
    glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3f");
    glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3fv");
    glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3s");
    glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib3sv");
    glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4Nbv");
    glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4Niv");
    glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4Nsv");
    glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4Nub");
    glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4Nubv");
    glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4Nuiv");
    glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4Nusv");
    glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4bv");
    glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4d");
    glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4dv");
    glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4f");
    glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4fv");
    glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4iv");
    glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4s");
    glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4sv");
    glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4ubv");
    glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4uiv");
    glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttrib4usv");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribPointer");

    // GL_VERSION_2_1
    if (mGLVersion < 2.09f)
    {
        return;
    }
    glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix2x3fv");
    glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3x2fv");
    glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix2x4fv");
    glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix4x2fv");
    glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3x4fv");
    glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix4x3fv");

    // GL_VERSION_3_0
    if (mGLVersion < 2.99f)
    {
        return;
    }
    glColorMaski = (PFNGLCOLORMASKIPROC)GLH_EXT_GET_PROC_ADDRESS("glColorMaski");
    glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBooleani_v");
    glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetIntegeri_v");
    glEnablei = (PFNGLENABLEIPROC)GLH_EXT_GET_PROC_ADDRESS("glEnablei");
    glDisablei = (PFNGLDISABLEIPROC)GLH_EXT_GET_PROC_ADDRESS("glDisablei");
    glIsEnabledi = (PFNGLISENABLEDIPROC)GLH_EXT_GET_PROC_ADDRESS("glIsEnabledi");
    glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glBeginTransformFeedback");
    glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glEndTransformFeedback");
    glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glBindBufferRange");
    glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)GLH_EXT_GET_PROC_ADDRESS("glBindBufferBase");
    glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)GLH_EXT_GET_PROC_ADDRESS("glTransformFeedbackVaryings");
    glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTransformFeedbackVarying");
    glClampColor = (PFNGLCLAMPCOLORPROC)GLH_EXT_GET_PROC_ADDRESS("glClampColor");
    glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)GLH_EXT_GET_PROC_ADDRESS("glBeginConditionalRender");
    glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)GLH_EXT_GET_PROC_ADDRESS("glEndConditionalRender");
    glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribIPointer");
    glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribIiv");
    glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribIuiv");
    glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI1i");
    glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI2i");
    glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI3i");
    glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4i");
    glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI1ui");
    glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI2ui");
    glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI3ui");
    glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4ui");
    glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI1iv");
    glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI2iv");
    glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI3iv");
    glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4iv");
    glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI1uiv");
    glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI2uiv");
    glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI3uiv");
    glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4uiv");
    glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4bv");
    glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4sv");
    glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4ubv");
    glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribI4usv");
    glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformuiv");
    glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)GLH_EXT_GET_PROC_ADDRESS("glBindFragDataLocation");
    glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)GLH_EXT_GET_PROC_ADDRESS("glGetFragDataLocation");
    glUniform1ui = (PFNGLUNIFORM1UIPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1ui");
    glUniform2ui = (PFNGLUNIFORM2UIPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2ui");
    glUniform3ui = (PFNGLUNIFORM3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3ui");
    glUniform4ui = (PFNGLUNIFORM4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4ui");
    glUniform1uiv = (PFNGLUNIFORM1UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1uiv");
    glUniform2uiv = (PFNGLUNIFORM2UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2uiv");
    glUniform3uiv = (PFNGLUNIFORM3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3uiv");
    glUniform4uiv = (PFNGLUNIFORM4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4uiv");
    glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTexParameterIiv");
    glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTexParameterIuiv");
    glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTexParameterIiv");
    glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTexParameterIuiv");
    glClearBufferiv = (PFNGLCLEARBUFFERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glClearBufferiv");
    glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glClearBufferuiv");
    glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glClearBufferfv");
    glClearBufferfi = (PFNGLCLEARBUFFERFIPROC)GLH_EXT_GET_PROC_ADDRESS("glClearBufferfi");
    glGetStringi = (PFNGLGETSTRINGIPROC)GLH_EXT_GET_PROC_ADDRESS("glGetStringi");
    glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glIsRenderbuffer");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glBindRenderbuffer");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteRenderbuffers");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glGenRenderbuffers");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glRenderbufferStorage");
    glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetRenderbufferParameteriv");
    glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glIsFramebuffer");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glBindFramebuffer");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteFramebuffers");
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glGenFramebuffers");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GLH_EXT_GET_PROC_ADDRESS("glCheckFramebufferStatus");
    glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glFramebufferTexture1D");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glFramebufferTexture2D");
    glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glFramebufferTexture3D");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glFramebufferRenderbuffer");
    glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetFramebufferAttachmentParameteriv");
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)GLH_EXT_GET_PROC_ADDRESS("glGenerateMipmap");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glBlitFramebuffer");
    glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glRenderbufferStorageMultisample");
    glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)GLH_EXT_GET_PROC_ADDRESS("glFramebufferTextureLayer");
    glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glMapBufferRange");
    glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glFlushMappedBufferRange");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GLH_EXT_GET_PROC_ADDRESS("glBindVertexArray");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteVertexArrays");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GLH_EXT_GET_PROC_ADDRESS("glGenVertexArrays");
    glIsVertexArray = (PFNGLISVERTEXARRAYPROC)GLH_EXT_GET_PROC_ADDRESS("glIsVertexArray");

    // GL_VERSION_3_1
    if (mGLVersion < 3.09f)
    {
        return;
    }
    glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawArraysInstanced");
    glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawElementsInstanced");
    glTexBuffer = (PFNGLTEXBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glTexBuffer");
    glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)GLH_EXT_GET_PROC_ADDRESS("glPrimitiveRestartIndex");
    glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glCopyBufferSubData");
    glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformIndices");
    glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveUniformsiv");
    glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveUniformName");
    glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformBlockIndex");
    glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveUniformBlockiv");
    glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveUniformBlockName");
    glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformBlockBinding");

    // GL_VERSION_3_2
    if (mGLVersion < 3.19f)
    {
        return;
    }
    glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawElementsBaseVertex");
    glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawRangeElementsBaseVertex");
    glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawElementsInstancedBaseVertex");
    glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiDrawElementsBaseVertex");
    glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)GLH_EXT_GET_PROC_ADDRESS("glProvokingVertex");
    glFenceSync = (PFNGLFENCESYNCPROC)GLH_EXT_GET_PROC_ADDRESS("glFenceSync");
    glIsSync = (PFNGLISSYNCPROC)GLH_EXT_GET_PROC_ADDRESS("glIsSync");
    glDeleteSync = (PFNGLDELETESYNCPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteSync");
    glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)GLH_EXT_GET_PROC_ADDRESS("glClientWaitSync");
    glWaitSync = (PFNGLWAITSYNCPROC)GLH_EXT_GET_PROC_ADDRESS("glWaitSync");
    glGetInteger64v = (PFNGLGETINTEGER64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetInteger64v");
    glGetSynciv = (PFNGLGETSYNCIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetSynciv");
    glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetInteger64i_v");
    glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetBufferParameteri64v");
    glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)GLH_EXT_GET_PROC_ADDRESS("glFramebufferTexture");
    glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glTexImage2DMultisample");
    glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glTexImage3DMultisample");
    glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetMultisamplefv");
    glSampleMaski = (PFNGLSAMPLEMASKIPROC)GLH_EXT_GET_PROC_ADDRESS("glSampleMaski");

    // GL_VERSION_3_3
    if (mGLVersion < 3.29f)
    {
        return;
    }
    glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)GLH_EXT_GET_PROC_ADDRESS("glBindFragDataLocationIndexed");
    glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)GLH_EXT_GET_PROC_ADDRESS("glGetFragDataIndex");
    glGenSamplers = (PFNGLGENSAMPLERSPROC)GLH_EXT_GET_PROC_ADDRESS("glGenSamplers");
    glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteSamplers");
    glIsSampler = (PFNGLISSAMPLERPROC)GLH_EXT_GET_PROC_ADDRESS("glIsSampler");
    glBindSampler = (PFNGLBINDSAMPLERPROC)GLH_EXT_GET_PROC_ADDRESS("glBindSampler");
    glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)GLH_EXT_GET_PROC_ADDRESS("glSamplerParameteri");
    glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glSamplerParameteriv");
    glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)GLH_EXT_GET_PROC_ADDRESS("glSamplerParameterf");
    glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glSamplerParameterfv");
    glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)GLH_EXT_GET_PROC_ADDRESS("glSamplerParameterIiv");
    glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glSamplerParameterIuiv");
    glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetSamplerParameteriv");
    glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetSamplerParameterIiv");
    glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetSamplerParameterfv");
    glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetSamplerParameterIuiv");
    glQueryCounter = (PFNGLQUERYCOUNTERPROC)GLH_EXT_GET_PROC_ADDRESS("glQueryCounter");
    glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjecti64v");
    glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryObjectui64v");
    glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribDivisor");
    glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP1ui");
    glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP1uiv");
    glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP2ui");
    glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP2uiv");
    glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP3ui");
    glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP3uiv");
    glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP4ui");
    glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribP4uiv");
    glVertexP2ui = (PFNGLVERTEXP2UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexP2ui");
    glVertexP2uiv = (PFNGLVERTEXP2UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexP2uiv");
    glVertexP3ui = (PFNGLVERTEXP3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexP3ui");
    glVertexP3uiv = (PFNGLVERTEXP3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexP3uiv");
    glVertexP4ui = (PFNGLVERTEXP4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexP4ui");
    glVertexP4uiv = (PFNGLVERTEXP4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexP4uiv");
    glTexCoordP1ui = (PFNGLTEXCOORDP1UIPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP1ui");
    glTexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP1uiv");
    glTexCoordP2ui = (PFNGLTEXCOORDP2UIPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP2ui");
    glTexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP2uiv");
    glTexCoordP3ui = (PFNGLTEXCOORDP3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP3ui");
    glTexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP3uiv");
    glTexCoordP4ui = (PFNGLTEXCOORDP4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP4ui");
    glTexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTexCoordP4uiv");
    glMultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP1ui");
    glMultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP1uiv");
    glMultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP2ui");
    glMultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP2uiv");
    glMultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP3ui");
    glMultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP3uiv");
    glMultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP4ui");
    glMultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiTexCoordP4uiv");
    glNormalP3ui = (PFNGLNORMALP3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glNormalP3ui");
    glNormalP3uiv = (PFNGLNORMALP3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glNormalP3uiv");
    glColorP3ui = (PFNGLCOLORP3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glColorP3ui");
    glColorP3uiv = (PFNGLCOLORP3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glColorP3uiv");
    glColorP4ui = (PFNGLCOLORP4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glColorP4ui");
    glColorP4uiv = (PFNGLCOLORP4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glColorP4uiv");
    glSecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColorP3ui");
    glSecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glSecondaryColorP3uiv");

    // GL_VERSION_4_0
    if (mGLVersion < 3.99f)
    {
        return;
    }
    glMinSampleShading = (PFNGLMINSAMPLESHADINGPROC)GLH_EXT_GET_PROC_ADDRESS("glMinSampleShading");
    glBlendEquationi = (PFNGLBLENDEQUATIONIPROC)GLH_EXT_GET_PROC_ADDRESS("glBlendEquationi");
    glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC)GLH_EXT_GET_PROC_ADDRESS("glBlendEquationSeparatei");
    glBlendFunci = (PFNGLBLENDFUNCIPROC)GLH_EXT_GET_PROC_ADDRESS("glBlendFunci");
    glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)GLH_EXT_GET_PROC_ADDRESS("glBlendFuncSeparatei");
    glDrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawArraysIndirect");
    glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawElementsIndirect");
    glUniform1d = (PFNGLUNIFORM1DPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1d");
    glUniform2d = (PFNGLUNIFORM2DPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2d");
    glUniform3d = (PFNGLUNIFORM3DPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3d");
    glUniform4d = (PFNGLUNIFORM4DPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4d");
    glUniform1dv = (PFNGLUNIFORM1DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform1dv");
    glUniform2dv = (PFNGLUNIFORM2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform2dv");
    glUniform3dv = (PFNGLUNIFORM3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform3dv");
    glUniform4dv = (PFNGLUNIFORM4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniform4dv");
    glUniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix2dv");
    glUniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3dv");
    glUniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix4dv");
    glUniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix2x3dv");
    glUniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix2x4dv");
    glUniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3x2dv");
    glUniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix3x4dv");
    glUniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix4x2dv");
    glUniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformMatrix4x3dv");
    glGetUniformdv = (PFNGLGETUNIFORMDVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformdv");
    glGetSubroutineUniformLocation = (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)GLH_EXT_GET_PROC_ADDRESS("glGetSubroutineUniformLocation");
    glGetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)GLH_EXT_GET_PROC_ADDRESS("glGetSubroutineIndex");
    glGetActiveSubroutineUniformiv = (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveSubroutineUniformiv");
    glGetActiveSubroutineUniformName = (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveSubroutineUniformName");
    glGetActiveSubroutineName = (PFNGLGETACTIVESUBROUTINENAMEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveSubroutineName");
    glUniformSubroutinesuiv = (PFNGLUNIFORMSUBROUTINESUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glUniformSubroutinesuiv");
    glGetUniformSubroutineuiv = (PFNGLGETUNIFORMSUBROUTINEUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetUniformSubroutineuiv");
    glGetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramStageiv");
    glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)GLH_EXT_GET_PROC_ADDRESS("glPatchParameteri");
    glPatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glPatchParameterfv");
    glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glBindTransformFeedback");
    glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteTransformFeedbacks");
    glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC)GLH_EXT_GET_PROC_ADDRESS("glGenTransformFeedbacks");
    glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glIsTransformFeedback");
    glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glPauseTransformFeedback");
    glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glResumeTransformFeedback");
    glDrawTransformFeedback = (PFNGLDRAWTRANSFORMFEEDBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawTransformFeedback");
    glDrawTransformFeedbackStream = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawTransformFeedbackStream");
    glBeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)GLH_EXT_GET_PROC_ADDRESS("glBeginQueryIndexed");
    glEndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)GLH_EXT_GET_PROC_ADDRESS("glEndQueryIndexed");
    glGetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryIndexediv");

    // GL_VERSION_4_1
    if (mGLVersion < 4.09f)
    {
        return;
    }
    glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)GLH_EXT_GET_PROC_ADDRESS("glReleaseShaderCompiler");
    glShaderBinary = (PFNGLSHADERBINARYPROC)GLH_EXT_GET_PROC_ADDRESS("glShaderBinary");
    glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)GLH_EXT_GET_PROC_ADDRESS("glGetShaderPrecisionFormat");
    glDepthRangef = (PFNGLDEPTHRANGEFPROC)GLH_EXT_GET_PROC_ADDRESS("glDepthRangef");
    glClearDepthf = (PFNGLCLEARDEPTHFPROC)GLH_EXT_GET_PROC_ADDRESS("glClearDepthf");
    glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramBinary");
    glProgramBinary = (PFNGLPROGRAMBINARYPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramBinary");
    glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramParameteri");
    glUseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)GLH_EXT_GET_PROC_ADDRESS("glUseProgramStages");
    glActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glActiveShaderProgram");
    glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateShaderProgramv");
    glBindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)GLH_EXT_GET_PROC_ADDRESS("glBindProgramPipeline");
    glDeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC)GLH_EXT_GET_PROC_ADDRESS("glDeleteProgramPipelines");
    glGenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)GLH_EXT_GET_PROC_ADDRESS("glGenProgramPipelines");
    glIsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)GLH_EXT_GET_PROC_ADDRESS("glIsProgramPipeline");
    glGetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramPipelineiv");
    glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1i");
    glProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1iv");
    glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1f");
    glProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1fv");
    glProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1d");
    glProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1dv");
    glProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1ui");
    glProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform1uiv");
    glProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2i");
    glProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2iv");
    glProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2f");
    glProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2fv");
    glProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2d");
    glProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2dv");
    glProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2ui");
    glProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform2uiv");
    glProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3i");
    glProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3iv");
    glProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3f");
    glProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3fv");
    glProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3d");
    glProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3dv");
    glProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3ui");
    glProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform3uiv");
    glProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4i");
    glProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4iv");
    glProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4f");
    glProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4fv");
    glProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4d");
    glProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4dv");
    glProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4ui");
    glProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniform4uiv");
    glProgramUniformMatrix2fv = (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix2fv");
    glProgramUniformMatrix3fv = (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix3fv");
    glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix4fv");
    glProgramUniformMatrix2dv = (PFNGLPROGRAMUNIFORMMATRIX2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix2dv");
    glProgramUniformMatrix3dv = (PFNGLPROGRAMUNIFORMMATRIX3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix3dv");
    glProgramUniformMatrix4dv = (PFNGLPROGRAMUNIFORMMATRIX4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix4dv");
    glProgramUniformMatrix2x3fv = (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix2x3fv");
    glProgramUniformMatrix3x2fv = (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix3x2fv");
    glProgramUniformMatrix2x4fv = (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix2x4fv");
    glProgramUniformMatrix4x2fv = (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix4x2fv");
    glProgramUniformMatrix3x4fv = (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix3x4fv");
    glProgramUniformMatrix4x3fv = (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix4x3fv");
    glProgramUniformMatrix2x3dv = (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix2x3dv");
    glProgramUniformMatrix3x2dv = (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix3x2dv");
    glProgramUniformMatrix2x4dv = (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix2x4dv");
    glProgramUniformMatrix4x2dv = (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix4x2dv");
    glProgramUniformMatrix3x4dv = (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix3x4dv");
    glProgramUniformMatrix4x3dv = (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glProgramUniformMatrix4x3dv");
    glValidateProgramPipeline = (PFNGLVALIDATEPROGRAMPIPELINEPROC)GLH_EXT_GET_PROC_ADDRESS("glValidateProgramPipeline");
    glGetProgramPipelineInfoLog = (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramPipelineInfoLog");
    glVertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL1d");
    glVertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL2d");
    glVertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL3d");
    glVertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL4d");
    glVertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL1dv");
    glVertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL2dv");
    glVertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL3dv");
    glVertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribL4dv");
    glVertexAttribLPointer = (PFNGLVERTEXATTRIBLPOINTERPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribLPointer");
    glGetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexAttribLdv");
    glViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)GLH_EXT_GET_PROC_ADDRESS("glViewportArrayv");
    glViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)GLH_EXT_GET_PROC_ADDRESS("glViewportIndexedf");
    glViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)GLH_EXT_GET_PROC_ADDRESS("glViewportIndexedfv");
    glScissorArrayv = (PFNGLSCISSORARRAYVPROC)GLH_EXT_GET_PROC_ADDRESS("glScissorArrayv");
    glScissorIndexed = (PFNGLSCISSORINDEXEDPROC)GLH_EXT_GET_PROC_ADDRESS("glScissorIndexed");
    glScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)GLH_EXT_GET_PROC_ADDRESS("glScissorIndexedv");
    glDepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC)GLH_EXT_GET_PROC_ADDRESS("glDepthRangeArrayv");
    glDepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC)GLH_EXT_GET_PROC_ADDRESS("glDepthRangeIndexed");
    glGetFloati_v = (PFNGLGETFLOATI_VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetFloati_v");
    glGetDoublei_v = (PFNGLGETDOUBLEI_VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetDoublei_v");

    // GL_VERSION_4_2
    if (mGLVersion < 4.19f)
    {
        return;
    }
    glDrawArraysInstancedBaseInstance = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawArraysInstancedBaseInstance");
    glDrawElementsInstancedBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawElementsInstancedBaseInstance");
    glDrawElementsInstancedBaseVertexBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawElementsInstancedBaseVertexBaseInstance");
    glGetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetInternalformativ");
    glGetActiveAtomicCounterBufferiv = (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetActiveAtomicCounterBufferiv");
    glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)GLH_EXT_GET_PROC_ADDRESS("glBindImageTexture");
    glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)GLH_EXT_GET_PROC_ADDRESS("glMemoryBarrier");
    glTexStorage1D = (PFNGLTEXSTORAGE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glTexStorage1D");
    glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glTexStorage2D");
    glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glTexStorage3D");
    glDrawTransformFeedbackInstanced = (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawTransformFeedbackInstanced");
    glDrawTransformFeedbackStreamInstanced = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)GLH_EXT_GET_PROC_ADDRESS("glDrawTransformFeedbackStreamInstanced");

    // GL_VERSION_4_3
    if (mGLVersion < 4.29f)
    {
        return;
    }
    glClearBufferData = (PFNGLCLEARBUFFERDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glClearBufferData");
    glClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glClearBufferSubData");
    glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)GLH_EXT_GET_PROC_ADDRESS("glDispatchCompute");
    glDispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC)GLH_EXT_GET_PROC_ADDRESS("glDispatchComputeIndirect");
    glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glCopyImageSubData");
    glFramebufferParameteri = (PFNGLFRAMEBUFFERPARAMETERIPROC)GLH_EXT_GET_PROC_ADDRESS("glFramebufferParameteri");
    glGetFramebufferParameteriv = (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetFramebufferParameteriv");
    glGetInternalformati64v = (PFNGLGETINTERNALFORMATI64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetInternalformati64v");
    glInvalidateTexSubImage = (PFNGLINVALIDATETEXSUBIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateTexSubImage");
    glInvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateTexImage");
    glInvalidateBufferSubData = (PFNGLINVALIDATEBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateBufferSubData");
    glInvalidateBufferData = (PFNGLINVALIDATEBUFFERDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateBufferData");
    glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateFramebuffer");
    glInvalidateSubFramebuffer = (PFNGLINVALIDATESUBFRAMEBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateSubFramebuffer");
    glMultiDrawArraysIndirect = (PFNGLMULTIDRAWARRAYSINDIRECTPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiDrawArraysIndirect");
    glMultiDrawElementsIndirect = (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiDrawElementsIndirect");
    glGetProgramInterfaceiv = (PFNGLGETPROGRAMINTERFACEIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramInterfaceiv");
    glGetProgramResourceIndex = (PFNGLGETPROGRAMRESOURCEINDEXPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramResourceIndex");
    glGetProgramResourceName = (PFNGLGETPROGRAMRESOURCENAMEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramResourceName");
    glGetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramResourceiv");
    glGetProgramResourceLocation = (PFNGLGETPROGRAMRESOURCELOCATIONPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramResourceLocation");
    glGetProgramResourceLocationIndex = (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)GLH_EXT_GET_PROC_ADDRESS("glGetProgramResourceLocationIndex");
    glShaderStorageBlockBinding = (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)GLH_EXT_GET_PROC_ADDRESS("glShaderStorageBlockBinding");
    glTexBufferRange = (PFNGLTEXBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glTexBufferRange");
    glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glTexStorage2DMultisample");
    glTexStorage3DMultisample = (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glTexStorage3DMultisample");
    glTextureView = (PFNGLTEXTUREVIEWPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureView");
    glBindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glBindVertexBuffer");
    glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribFormat");
    glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribIFormat");
    glVertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribLFormat");
    glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexAttribBinding");
    glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexBindingDivisor");
    glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)GLH_EXT_GET_PROC_ADDRESS("glDebugMessageControl");
    glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)GLH_EXT_GET_PROC_ADDRESS("glDebugMessageInsert");
    glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)GLH_EXT_GET_PROC_ADDRESS("glDebugMessageCallback");
    glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)GLH_EXT_GET_PROC_ADDRESS("glGetDebugMessageLog");
    glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)GLH_EXT_GET_PROC_ADDRESS("glPushDebugGroup");
    glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)GLH_EXT_GET_PROC_ADDRESS("glPopDebugGroup");
    glObjectLabel = (PFNGLOBJECTLABELPROC)GLH_EXT_GET_PROC_ADDRESS("glObjectLabel");
    glGetObjectLabel = (PFNGLGETOBJECTLABELPROC)GLH_EXT_GET_PROC_ADDRESS("glGetObjectLabel");
    glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)GLH_EXT_GET_PROC_ADDRESS("glObjectPtrLabel");
    glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)GLH_EXT_GET_PROC_ADDRESS("glGetObjectPtrLabel");

    // GL_VERSION_4_4
    if (mGLVersion < 4.39f)
    {
        return;
    }
    glBufferStorage = (PFNGLBUFFERSTORAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glBufferStorage");
    glClearTexImage = (PFNGLCLEARTEXIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glClearTexImage");
    glClearTexSubImage = (PFNGLCLEARTEXSUBIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glClearTexSubImage");
    glBindBuffersBase = (PFNGLBINDBUFFERSBASEPROC)GLH_EXT_GET_PROC_ADDRESS("glBindBuffersBase");
    glBindBuffersRange = (PFNGLBINDBUFFERSRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glBindBuffersRange");
    glBindTextures = (PFNGLBINDTEXTURESPROC)GLH_EXT_GET_PROC_ADDRESS("glBindTextures");
    glBindSamplers = (PFNGLBINDSAMPLERSPROC)GLH_EXT_GET_PROC_ADDRESS("glBindSamplers");
    glBindImageTextures = (PFNGLBINDIMAGETEXTURESPROC)GLH_EXT_GET_PROC_ADDRESS("glBindImageTextures");
    glBindVertexBuffers = (PFNGLBINDVERTEXBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glBindVertexBuffers");

    // GL_VERSION_4_5
    if (mGLVersion < 4.49f)
    {
        return;
    }
    glClipControl = (PFNGLCLIPCONTROLPROC)GLH_EXT_GET_PROC_ADDRESS("glClipControl");
    glCreateTransformFeedbacks = (PFNGLCREATETRANSFORMFEEDBACKSPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateTransformFeedbacks");
    glTransformFeedbackBufferBase = (PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC)GLH_EXT_GET_PROC_ADDRESS("glTransformFeedbackBufferBase");
    glTransformFeedbackBufferRange = (PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glTransformFeedbackBufferRange");
    glGetTransformFeedbackiv = (PFNGLGETTRANSFORMFEEDBACKIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTransformFeedbackiv");
    glGetTransformFeedbacki_v = (PFNGLGETTRANSFORMFEEDBACKI_VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTransformFeedbacki_v");
    glGetTransformFeedbacki64_v = (PFNGLGETTRANSFORMFEEDBACKI64_VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTransformFeedbacki64_v");
    glCreateBuffers = (PFNGLCREATEBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateBuffers");
    glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedBufferStorage");
    glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedBufferData");
    glNamedBufferSubData = (PFNGLNAMEDBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedBufferSubData");
    glCopyNamedBufferSubData = (PFNGLCOPYNAMEDBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glCopyNamedBufferSubData");
    glClearNamedBufferData = (PFNGLCLEARNAMEDBUFFERDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glClearNamedBufferData");
    glClearNamedBufferSubData = (PFNGLCLEARNAMEDBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glClearNamedBufferSubData");
    glMapNamedBuffer = (PFNGLMAPNAMEDBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glMapNamedBuffer");
    glMapNamedBufferRange = (PFNGLMAPNAMEDBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glMapNamedBufferRange");
    glUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glUnmapNamedBuffer");
    glFlushMappedNamedBufferRange = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glFlushMappedNamedBufferRange");
    glGetNamedBufferParameteriv = (PFNGLGETNAMEDBUFFERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetNamedBufferParameteriv");
    glGetNamedBufferParameteri64v = (PFNGLGETNAMEDBUFFERPARAMETERI64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetNamedBufferParameteri64v");
    glGetNamedBufferPointerv = (PFNGLGETNAMEDBUFFERPOINTERVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetNamedBufferPointerv");
    glGetNamedBufferSubData = (PFNGLGETNAMEDBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glGetNamedBufferSubData");
    glCreateFramebuffers = (PFNGLCREATEFRAMEBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateFramebuffers");
    glNamedFramebufferRenderbuffer = (PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedFramebufferRenderbuffer");
    glNamedFramebufferParameteri = (PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedFramebufferParameteri");
    glNamedFramebufferTexture = (PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedFramebufferTexture");
    glNamedFramebufferTextureLayer = (PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedFramebufferTextureLayer");
    glNamedFramebufferDrawBuffer = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedFramebufferDrawBuffer");
    glNamedFramebufferDrawBuffers = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedFramebufferDrawBuffers");
    glNamedFramebufferReadBuffer = (PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedFramebufferReadBuffer");
    glInvalidateNamedFramebufferData = (PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateNamedFramebufferData");
    glInvalidateNamedFramebufferSubData = (PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC)GLH_EXT_GET_PROC_ADDRESS("glInvalidateNamedFramebufferSubData");
    glClearNamedFramebufferiv = (PFNGLCLEARNAMEDFRAMEBUFFERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glClearNamedFramebufferiv");
    glClearNamedFramebufferuiv = (PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glClearNamedFramebufferuiv");
    glClearNamedFramebufferfv = (PFNGLCLEARNAMEDFRAMEBUFFERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glClearNamedFramebufferfv");
    glClearNamedFramebufferfi = (PFNGLCLEARNAMEDFRAMEBUFFERFIPROC)GLH_EXT_GET_PROC_ADDRESS("glClearNamedFramebufferfi");
    glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glBlitNamedFramebuffer");
    glCheckNamedFramebufferStatus = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)GLH_EXT_GET_PROC_ADDRESS("glCheckNamedFramebufferStatus");
    glGetNamedFramebufferParameteriv = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetNamedFramebufferParameteriv");
    glGetNamedFramebufferAttachmentParameteriv = (PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetNamedFramebufferAttachmentParameteriv");
    glCreateRenderbuffers = (PFNGLCREATERENDERBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateRenderbuffers");
    glNamedRenderbufferStorage = (PFNGLNAMEDRENDERBUFFERSTORAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedRenderbufferStorage");
    glNamedRenderbufferStorageMultisample = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glNamedRenderbufferStorageMultisample");
    glGetNamedRenderbufferParameteriv = (PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetNamedRenderbufferParameteriv");
    glCreateTextures = (PFNGLCREATETEXTURESPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateTextures");
    glTextureBuffer = (PFNGLTEXTUREBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureBuffer");
    glTextureBufferRange = (PFNGLTEXTUREBUFFERRANGEPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureBufferRange");
    glTextureStorage1D = (PFNGLTEXTURESTORAGE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureStorage1D");
    glTextureStorage2D = (PFNGLTEXTURESTORAGE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureStorage2D");
    glTextureStorage3D = (PFNGLTEXTURESTORAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureStorage3D");
    glTextureStorage2DMultisample = (PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureStorage2DMultisample");
    glTextureStorage3DMultisample = (PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureStorage3DMultisample");
    glTextureSubImage1D = (PFNGLTEXTURESUBIMAGE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureSubImage1D");
    glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureSubImage2D");
    glTextureSubImage3D = (PFNGLTEXTURESUBIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureSubImage3D");
    glCompressedTextureSubImage1D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTextureSubImage1D");
    glCompressedTextureSubImage2D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTextureSubImage2D");
    glCompressedTextureSubImage3D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glCompressedTextureSubImage3D");
    glCopyTextureSubImage1D = (PFNGLCOPYTEXTURESUBIMAGE1DPROC)GLH_EXT_GET_PROC_ADDRESS("glCopyTextureSubImage1D");
    glCopyTextureSubImage2D = (PFNGLCOPYTEXTURESUBIMAGE2DPROC)GLH_EXT_GET_PROC_ADDRESS("glCopyTextureSubImage2D");
    glCopyTextureSubImage3D = (PFNGLCOPYTEXTURESUBIMAGE3DPROC)GLH_EXT_GET_PROC_ADDRESS("glCopyTextureSubImage3D");
    glTextureParameterf = (PFNGLTEXTUREPARAMETERFPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureParameterf");
    glTextureParameterfv = (PFNGLTEXTUREPARAMETERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureParameterfv");
    glTextureParameteri = (PFNGLTEXTUREPARAMETERIPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureParameteri");
    glTextureParameterIiv = (PFNGLTEXTUREPARAMETERIIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureParameterIiv");
    glTextureParameterIuiv = (PFNGLTEXTUREPARAMETERIUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureParameterIuiv");
    glTextureParameteriv = (PFNGLTEXTUREPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureParameteriv");
    glGenerateTextureMipmap = (PFNGLGENERATETEXTUREMIPMAPPROC)GLH_EXT_GET_PROC_ADDRESS("glGenerateTextureMipmap");
    glBindTextureUnit = (PFNGLBINDTEXTUREUNITPROC)GLH_EXT_GET_PROC_ADDRESS("glBindTextureUnit");
    glGetTextureImage = (PFNGLGETTEXTUREIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureImage");
    glGetCompressedTextureImage = (PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetCompressedTextureImage");
    glGetTextureLevelParameterfv = (PFNGLGETTEXTURELEVELPARAMETERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureLevelParameterfv");
    glGetTextureLevelParameteriv = (PFNGLGETTEXTURELEVELPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureLevelParameteriv");
    glGetTextureParameterfv = (PFNGLGETTEXTUREPARAMETERFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureParameterfv");
    glGetTextureParameterIiv = (PFNGLGETTEXTUREPARAMETERIIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureParameterIiv");
    glGetTextureParameterIuiv = (PFNGLGETTEXTUREPARAMETERIUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureParameterIuiv");
    glGetTextureParameteriv = (PFNGLGETTEXTUREPARAMETERIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureParameteriv");
    glCreateVertexArrays = (PFNGLCREATEVERTEXARRAYSPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateVertexArrays");
    glDisableVertexArrayAttrib = (PFNGLDISABLEVERTEXARRAYATTRIBPROC)GLH_EXT_GET_PROC_ADDRESS("glDisableVertexArrayAttrib");
    glEnableVertexArrayAttrib = (PFNGLENABLEVERTEXARRAYATTRIBPROC)GLH_EXT_GET_PROC_ADDRESS("glEnableVertexArrayAttrib");
    glVertexArrayElementBuffer = (PFNGLVERTEXARRAYELEMENTBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayElementBuffer");
    glVertexArrayVertexBuffer = (PFNGLVERTEXARRAYVERTEXBUFFERPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayVertexBuffer");
    glVertexArrayVertexBuffers = (PFNGLVERTEXARRAYVERTEXBUFFERSPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayVertexBuffers");
    glVertexArrayAttribBinding = (PFNGLVERTEXARRAYATTRIBBINDINGPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayAttribBinding");
    glVertexArrayAttribFormat = (PFNGLVERTEXARRAYATTRIBFORMATPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayAttribFormat");
    glVertexArrayAttribIFormat = (PFNGLVERTEXARRAYATTRIBIFORMATPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayAttribIFormat");
    glVertexArrayAttribLFormat = (PFNGLVERTEXARRAYATTRIBLFORMATPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayAttribLFormat");
    glVertexArrayBindingDivisor = (PFNGLVERTEXARRAYBINDINGDIVISORPROC)GLH_EXT_GET_PROC_ADDRESS("glVertexArrayBindingDivisor");
    glGetVertexArrayiv = (PFNGLGETVERTEXARRAYIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexArrayiv");
    glGetVertexArrayIndexediv = (PFNGLGETVERTEXARRAYINDEXEDIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexArrayIndexediv");
    glGetVertexArrayIndexed64iv = (PFNGLGETVERTEXARRAYINDEXED64IVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetVertexArrayIndexed64iv");
    glCreateSamplers = (PFNGLCREATESAMPLERSPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateSamplers");
    glCreateProgramPipelines = (PFNGLCREATEPROGRAMPIPELINESPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateProgramPipelines");
    glCreateQueries = (PFNGLCREATEQUERIESPROC)GLH_EXT_GET_PROC_ADDRESS("glCreateQueries");
    glGetQueryBufferObjecti64v = (PFNGLGETQUERYBUFFEROBJECTI64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryBufferObjecti64v");
    glGetQueryBufferObjectiv = (PFNGLGETQUERYBUFFEROBJECTIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryBufferObjectiv");
    glGetQueryBufferObjectui64v = (PFNGLGETQUERYBUFFEROBJECTUI64VPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryBufferObjectui64v");
    glGetQueryBufferObjectuiv = (PFNGLGETQUERYBUFFEROBJECTUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetQueryBufferObjectuiv");
    glMemoryBarrierByRegion = (PFNGLMEMORYBARRIERBYREGIONPROC)GLH_EXT_GET_PROC_ADDRESS("glMemoryBarrierByRegion");
    glGetTextureSubImage = (PFNGLGETTEXTURESUBIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetTextureSubImage");
    glGetCompressedTextureSubImage = (PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetCompressedTextureSubImage");
    glGetGraphicsResetStatus = (PFNGLGETGRAPHICSRESETSTATUSPROC)GLH_EXT_GET_PROC_ADDRESS("glGetGraphicsResetStatus");
    glGetnCompressedTexImage = (PFNGLGETNCOMPRESSEDTEXIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnCompressedTexImage");
    glGetnTexImage = (PFNGLGETNTEXIMAGEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnTexImage");
    glGetnUniformdv = (PFNGLGETNUNIFORMDVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnUniformdv");
    glGetnUniformfv = (PFNGLGETNUNIFORMFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnUniformfv");
    glGetnUniformiv = (PFNGLGETNUNIFORMIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnUniformiv");
    glGetnUniformuiv = (PFNGLGETNUNIFORMUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnUniformuiv");
    glReadnPixels = (PFNGLREADNPIXELSPROC)GLH_EXT_GET_PROC_ADDRESS("glReadnPixels");
    glGetnMapdv = (PFNGLGETNMAPDVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnMapdv");
    glGetnMapfv = (PFNGLGETNMAPFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnMapfv");
    glGetnMapiv = (PFNGLGETNMAPIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnMapiv");
    glGetnPixelMapfv = (PFNGLGETNPIXELMAPFVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnPixelMapfv");
    glGetnPixelMapuiv = (PFNGLGETNPIXELMAPUIVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnPixelMapuiv");
    glGetnPixelMapusv = (PFNGLGETNPIXELMAPUSVPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnPixelMapusv");
    glGetnPolygonStipple = (PFNGLGETNPOLYGONSTIPPLEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnPolygonStipple");
    glGetnColorTable = (PFNGLGETNCOLORTABLEPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnColorTable");
    glGetnConvolutionFilter = (PFNGLGETNCONVOLUTIONFILTERPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnConvolutionFilter");
    glGetnSeparableFilter = (PFNGLGETNSEPARABLEFILTERPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnSeparableFilter");
    glGetnHistogram = (PFNGLGETNHISTOGRAMPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnHistogram");
    glGetnMinmax = (PFNGLGETNMINMAXPROC)GLH_EXT_GET_PROC_ADDRESS("glGetnMinmax");
    glTextureBarrier = (PFNGLTEXTUREBARRIERPROC)GLH_EXT_GET_PROC_ADDRESS("glTextureBarrier");

    // GL_VERSION_4_6
    if (mGLVersion < 4.59f)
    {
        return;
    }
    glSpecializeShader = (PFNGLSPECIALIZESHADERPROC)GLH_EXT_GET_PROC_ADDRESS("glSpecializeShader");
    glMultiDrawArraysIndirectCount = (PFNGLMULTIDRAWARRAYSINDIRECTCOUNTPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiDrawArraysIndirectCount");
    glMultiDrawElementsIndirectCount = (PFNGLMULTIDRAWELEMENTSINDIRECTCOUNTPROC)GLH_EXT_GET_PROC_ADDRESS("glMultiDrawElementsIndirectCount");
    glPolygonOffsetClamp = (PFNGLPOLYGONOFFSETCLAMPPROC)GLH_EXT_GET_PROC_ADDRESS("glPolygonOffsetClamp");
    
#endif
}

void rotate_quat(LLQuaternion& rotation)
{
	F32 angle_radians, x, y, z;
	rotation.getAngleAxis(&angle_radians, &x, &y, &z);
	gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);
}

void flush_glerror()
{
	glGetError();
}

//this function outputs gl error to the log file, does not crash the code.
void log_glerror()
{
	if (LL_UNLIKELY(!gGLManager.mInited))
	{
		return ;
	}
	//  Create or update texture to be used with this data 
	GLenum error;
	error = glGetError();
	while (LL_UNLIKELY(error))
	{
		GLubyte const * gl_error_msg = gluErrorString(error);
		if (NULL != gl_error_msg)
		{
			LL_WARNS() << "GL Error: " << error << " GL Error String: " << gl_error_msg << LL_ENDL ;			
		}
		else
		{
			// gluErrorString returns NULL for some extensions' error codes.
			// you'll probably have to grep for the number in glext.h.
			LL_WARNS() << "GL Error: UNKNOWN 0x" << std::hex << error << std::dec << LL_ENDL;
		}
		error = glGetError();
	}
}

void do_assert_glerror()
{
	//  Create or update texture to be used with this data 
	GLenum error;
	error = glGetError();
	bool quit = false;
	if (LL_UNLIKELY(error))
	{
		quit = true;
		GLubyte const * gl_error_msg = gluErrorString(error);
		if (NULL != gl_error_msg)
		{
			LL_WARNS("RenderState") << "GL Error:" << error<< LL_ENDL;
			LL_WARNS("RenderState") << "GL Error String:" << gl_error_msg << LL_ENDL;

			if (gDebugSession)
			{
				gFailLog << "GL Error:" << gl_error_msg << std::endl;
			}
		}
		else
		{
			// gluErrorString returns NULL for some extensions' error codes.
			// you'll probably have to grep for the number in glext.h.
			LL_WARNS("RenderState") << "GL Error: UNKNOWN 0x" << std::hex << error << std::dec << LL_ENDL;

			if (gDebugSession)
			{
				gFailLog << "GL Error: UNKNOWN 0x" << std::hex << error << std::dec << std::endl;
			}
		}
	}

	if (quit)
	{
		if (gDebugSession)
		{
			ll_fail("assert_glerror failed");
		}
		else
		{
			LL_ERRS() << "One or more unhandled GL errors." << LL_ENDL;
		}
	}
}

void assert_glerror()
{
/*	if (!gGLActive)
	{
		//LL_WARNS() << "GL used while not active!" << LL_ENDL;

		if (gDebugSession)
		{
			//ll_fail("GL used while not active");
		}
	}
*/

	if (!gDebugGL) 
	{
		//funny looking if for branch prediction -- gDebugGL is almost always false and assert_glerror is called often
	}
	else
	{
		do_assert_glerror();
	}
}
	

void clear_glerror()
{
	glGetError();
	glGetError();
}

///////////////////////////////////////////////////////////////
//
// LLGLState
//

// Static members
boost::unordered_map<LLGLenum, LLGLboolean> LLGLState::sStateMap;

GLboolean LLGLDepthTest::sDepthEnabled = GL_FALSE; // OpenGL default
GLenum LLGLDepthTest::sDepthFunc = GL_LESS; // OpenGL default
GLboolean LLGLDepthTest::sWriteEnabled = GL_TRUE; // OpenGL default

//static
void LLGLState::initClass() 
{
	sStateMap[GL_DITHER] = GL_TRUE;
	// sStateMap[GL_TEXTURE_2D] = GL_TRUE;
	
	//make sure multisample defaults to disabled
	sStateMap[GL_MULTISAMPLE] = GL_FALSE;
	glDisable(GL_MULTISAMPLE);
}

//static
void LLGLState::restoreGL()
{
	sStateMap.clear();
	initClass();
}

//static
// Really shouldn't be needed, but seems we sometimes do.
void LLGLState::resetTextureStates()
{
	gGL.flush();
	GLint maxTextureUnits;
	
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &maxTextureUnits);
	for (S32 j = maxTextureUnits-1; j >=0; j--)
	{
		gGL.getTexUnit(j)->activate();
		glClientActiveTexture(GL_TEXTURE0+j);
		j == 0 ? gGL.getTexUnit(j)->enable(LLTexUnit::TT_TEXTURE) : gGL.getTexUnit(j)->disable();
	}
}

void LLGLState::dumpStates() 
{
	LL_INFOS("RenderState") << "GL States:" << LL_ENDL;
	for (boost::unordered_map<LLGLenum, LLGLboolean>::iterator iter = sStateMap.begin();
		 iter != sStateMap.end(); ++iter)
	{
		LL_INFOS("RenderState") << llformat(" 0x%04x : %s",(S32)iter->first,iter->second?"true":"false") << LL_ENDL;
	}
}

void LLGLState::checkStates(GLboolean writeAlpha)
{
	if (!gDebugGL)
	{
		return;
	}

	GLint src;
	GLint dst;
	glGetIntegerv(GL_BLEND_SRC, &src);
	glGetIntegerv(GL_BLEND_DST, &dst);
    llassert_always(src == GL_SRC_ALPHA);
    llassert_always(dst == GL_ONE_MINUS_SRC_ALPHA);
  
    // disable for now until usage is consistent
    //GLboolean colorMask[4];
    //glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    //llassert_always(colorMask[0]);
    //llassert_always(colorMask[1]);
    //llassert_always(colorMask[2]);
    // llassert_always(colorMask[3] == writeAlpha);  
	
	for (boost::unordered_map<LLGLenum, LLGLboolean>::iterator iter = sStateMap.begin();
		 iter != sStateMap.end(); ++iter)
	{
		LLGLenum state = iter->first;
		LLGLboolean cur_state = iter->second;
		LLGLboolean gl_state = glIsEnabled(state);
		if(cur_state != gl_state)
		{
			dumpStates();
			LL_GL_ERRS << llformat("LLGLState error. State: 0x%04x",state) << LL_ENDL;
		}
	}
}

///////////////////////////////////////////////////////////////////////

LLGLState::LLGLState(LLGLenum state, S32 enabled) :
	mState(state), mWasEnabled(false), mIsEnabled(false)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

	if (mState)
	{
		mWasEnabled = sStateMap[state];
		setEnabled(enabled);
	}
}

void LLGLState::setEnabled(S32 enabled)
{
	if (!mState)
	{
		return;
	}
	if (enabled == CURRENT_STATE)
	{
		enabled = sStateMap[mState] == GL_TRUE ? ENABLED_STATE : DISABLED_STATE;
	}
	else if (enabled == ENABLED_STATE && sStateMap[mState] != GL_TRUE)
	{
		gGL.flush();
		glEnable(mState);
		sStateMap[mState] = GL_TRUE;
	}
	else if (enabled == DISABLED_STATE && sStateMap[mState] != GL_FALSE)
	{
		gGL.flush();
		glDisable(mState);
		sStateMap[mState] = GL_FALSE;
	}
	mIsEnabled = enabled;
}

LLGLState::~LLGLState() 
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	if (mState)
	{
		if (gDebugGL)
		{
			if (!gDebugSession)
			{
				llassert_always(sStateMap[mState] == glIsEnabled(mState));
			}
			else
			{
				if (sStateMap[mState] != glIsEnabled(mState))
				{
					ll_fail("GL enabled state does not match expected");
				}
			}
		}

		if (mIsEnabled != mWasEnabled)
		{
			gGL.flush();
			if (mWasEnabled)
			{
				glEnable(mState);
				sStateMap[mState] = GL_TRUE;
			}
			else
			{
				glDisable(mState);
				sStateMap[mState] = GL_FALSE;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void LLGLManager::initGLStates()
{
	//gl states moved to classes in llglstates.h
	LLGLState::initClass();
}

////////////////////////////////////////////////////////////////////////////////

void parse_gl_version( S32* major, S32* minor, S32* release, std::string* vendor_specific, std::string* version_string )
{
	// GL_VERSION returns a null-terminated string with the format: 
	// <major>.<minor>[.<release>] [<vendor specific>]

	const char* version = (const char*) glGetString(GL_VERSION);
	*major = 0;
	*minor = 0;
	*release = 0;
	vendor_specific->assign("");

	if( !version )
	{
		return;
	}

	version_string->assign(version);

	std::string ver_copy( version );
	S32 len = (S32)strlen( version );	/* Flawfinder: ignore */
	S32 i = 0;
	S32 start;
	// Find the major version
	start = i;
	for( ; i < len; i++ )
	{
		if( '.' == version[i] )
		{
			break;
		}
	}
	std::string major_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(major_str, *major);

	if( '.' == version[i] )
	{
		i++;
	}

	// Find the minor version
	start = i;
	for( ; i < len; i++ )
	{
		if( ('.' == version[i]) || isspace(version[i]) )
		{
			break;
		}
	}
	std::string minor_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(minor_str, *minor);

	// Find the release number (optional)
	if( '.' == version[i] )
	{
		i++;

		start = i;
		for( ; i < len; i++ )
		{
			if( isspace(version[i]) )
			{
				break;
			}
		}

		std::string release_str = ver_copy.substr(start,i-start);
		LLStringUtil::convertToS32(release_str, *release);
	}

	// Skip over any white space
	while( version[i] && isspace( version[i] ) )
	{
		i++;
	}

	// Copy the vendor-specific string (optional)
	if( version[i] )
	{
		vendor_specific->assign( version + i );
	}
}


void parse_glsl_version(S32& major, S32& minor)
{
	// GL_SHADING_LANGUAGE_VERSION returns a null-terminated string with the format: 
	// <major>.<minor>[.<release>] [<vendor specific>]

	const char* version = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
	major = 0;
	minor = 0;
	
	if( !version )
	{
		return;
	}

	std::string ver_copy( version );
	S32 len = (S32)strlen( version );	/* Flawfinder: ignore */
	S32 i = 0;
	S32 start;
	// Find the major version
	start = i;
	for( ; i < len; i++ )
	{
		if( '.' == version[i] )
		{
			break;
		}
	}
	std::string major_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(major_str, major);

	if( '.' == version[i] )
	{
		i++;
	}

	// Find the minor version
	start = i;
	for( ; i < len; i++ )
	{
		if( ('.' == version[i]) || isspace(version[i]) )
		{
			break;
		}
	}
	std::string minor_str = ver_copy.substr(start,i-start);
	LLStringUtil::convertToS32(minor_str, minor);
}

LLGLUserClipPlane::LLGLUserClipPlane(const LLPlane& p, const glh::matrix4f& modelview, const glh::matrix4f& projection, bool apply)
{
	mApply = apply;

	if (mApply)
	{
		mModelview = modelview;
		mProjection = projection;

        //flip incoming LLPlane to get consistent behavior compared to frustum culling
		setPlane(-p[0], -p[1], -p[2], -p[3]);
	}
}

void LLGLUserClipPlane::disable()
{
    if (mApply)
	{
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.popMatrix();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
    mApply = false;
}

void LLGLUserClipPlane::setPlane(F32 a, F32 b, F32 c, F32 d)
{
	glh::matrix4f& P = mProjection;
	glh::matrix4f& M = mModelview;
    
	glh::matrix4f invtrans_MVP = (P * M).inverse().transpose();
    glh::vec4f oplane(a,b,c,d);
    glh::vec4f cplane;
    invtrans_MVP.mult_matrix_vec(oplane, cplane);

    cplane /= fabs(cplane[2]); // normalize such that depth is not scaled
    cplane[3] -= 1;

    if(cplane[2] < 0)
        cplane *= -1;

    glh::matrix4f suffix;
    suffix.set_row(2, cplane);
    glh::matrix4f newP = suffix * P;
    gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
    gGL.loadMatrix(newP.m);
	gGLObliqueProjectionInverse = LLMatrix4(newP.inverse().transpose().m);
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}

LLGLUserClipPlane::~LLGLUserClipPlane()
{
	disable();
}

LLGLDepthTest::LLGLDepthTest(GLboolean depth_enabled, GLboolean write_enabled, GLenum depth_func)
: mPrevDepthEnabled(sDepthEnabled), mPrevDepthFunc(sDepthFunc), mPrevWriteEnabled(sWriteEnabled)
{
	stop_glerror();
	
	checkState();

	if (!depth_enabled)
	{ // always disable depth writes if depth testing is disabled
	  // GL spec defines this as a requirement, but some implementations allow depth writes with testing disabled
	  // The proper way to write to depth buffer with testing disabled is to enable testing and use a depth_func of GL_ALWAYS
		write_enabled = GL_FALSE;
	}

	if (depth_enabled != sDepthEnabled)
	{
		gGL.flush();
		if (depth_enabled) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
		sDepthEnabled = depth_enabled;
	}
	if (depth_func != sDepthFunc)
	{
		gGL.flush();
		glDepthFunc(depth_func);
		sDepthFunc = depth_func;
	}
	if (write_enabled != sWriteEnabled)
	{
		gGL.flush();
		glDepthMask(write_enabled);
		sWriteEnabled = write_enabled;
	}
}

LLGLDepthTest::~LLGLDepthTest()
{
	checkState();
	if (sDepthEnabled != mPrevDepthEnabled )
	{
		gGL.flush();
		if (mPrevDepthEnabled) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
		sDepthEnabled = mPrevDepthEnabled;
	}
	if (sDepthFunc != mPrevDepthFunc)
	{
		gGL.flush();
		glDepthFunc(mPrevDepthFunc);
		sDepthFunc = mPrevDepthFunc;
	}
	if (sWriteEnabled != mPrevWriteEnabled )
	{
		gGL.flush();
		glDepthMask(mPrevWriteEnabled);
		sWriteEnabled = mPrevWriteEnabled;
	}
}

void LLGLDepthTest::checkState()
{
	if (gDebugGL)
	{
		GLint func = 0;
		GLboolean mask = GL_FALSE;

		glGetIntegerv(GL_DEPTH_FUNC, &func);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);

		if (glIsEnabled(GL_DEPTH_TEST) != sDepthEnabled ||
			sWriteEnabled != mask ||
			sDepthFunc != func)
		{
			if (gDebugSession)
			{
				gFailLog << "Unexpected depth testing state." << std::endl;
			}
			else
			{
				LL_GL_ERRS << "Unexpected depth testing state." << LL_ENDL;
			}
		}
	}
}

LLGLSquashToFarClip::LLGLSquashToFarClip()
{
    glh::matrix4f proj = get_current_projection();
    setProjectionMatrix(proj, 0);
}

LLGLSquashToFarClip::LLGLSquashToFarClip(glh::matrix4f& P, U32 layer)
{
    setProjectionMatrix(P, layer);
}


void LLGLSquashToFarClip::setProjectionMatrix(glh::matrix4f& projection, U32 layer)
{

	F32 depth = 0.99999f - 0.0001f * layer;

	for (U32 i = 0; i < 4; i++)
	{
		projection.element(2, i) = projection.element(3, i) * depth;
	}

    LLRender::eMatrixMode last_matrix_mode = gGL.getMatrixMode();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadMatrix(projection.m);

	gGL.matrixMode(last_matrix_mode);
}

LLGLSquashToFarClip::~LLGLSquashToFarClip()
{
    LLRender::eMatrixMode last_matrix_mode = gGL.getMatrixMode();

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();

	gGL.matrixMode(last_matrix_mode);
}


	
LLGLSyncFence::LLGLSyncFence()
{
	mSync = 0;
}

LLGLSyncFence::~LLGLSyncFence()
{
	if (mSync)
	{
		glDeleteSync(mSync);
	}
}

void LLGLSyncFence::placeFence()
{
	if (mSync)
	{
		glDeleteSync(mSync);
	}
	mSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

bool LLGLSyncFence::isCompleted()
{
	bool ret = true;
	if (mSync)
	{
		GLenum status = glClientWaitSync(mSync, 0, 1);
		if (status == GL_TIMEOUT_EXPIRED)
		{
			ret = false;
		}
	}
	return ret;
}

void LLGLSyncFence::wait()
{
	if (mSync)
	{
		while (glClientWaitSync(mSync, 0, FENCE_WAIT_TIME_NANOSECONDS) == GL_TIMEOUT_EXPIRED)
		{ //track the number of times we've waited here
		}
	}
}

LLGLSPipelineSkyBox::LLGLSPipelineSkyBox()
: mCullFace(GL_CULL_FACE)
, mSquashClip()
{ 
}

LLGLSPipelineSkyBox::~LLGLSPipelineSkyBox()
{
}

LLGLSPipelineDepthTestSkyBox::LLGLSPipelineDepthTestSkyBox(bool depth_test, bool depth_write)
: LLGLSPipelineSkyBox()
, mDepth(depth_test ? GL_TRUE : GL_FALSE, depth_write ? GL_TRUE : GL_FALSE, GL_LEQUAL)
{

}

LLGLSPipelineBlendSkyBox::LLGLSPipelineBlendSkyBox(bool depth_test, bool depth_write)
: LLGLSPipelineDepthTestSkyBox(depth_test, depth_write)    
, mBlend(GL_BLEND)
{ 
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

#if LL_WINDOWS
// Expose desired use of high-performance graphics processor to Optimus driver and to AMD driver
// https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
extern "C" 
{ 
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


